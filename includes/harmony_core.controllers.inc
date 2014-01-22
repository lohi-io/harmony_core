<?php

/**
 * @file
 * Class definitions for the Thread and Post entity.
 */

class HarmonyThreadController extends EntityAPIController {

  /*public function create(array $values = array()) {
    global $user;
    $values += array(
      'title' => '',
      'description' => '',
      'created' => REQUEST_TIME,
      'changed' => REQUEST_TIME,
      'uid' => $user->uid,
    );
    return parent::create($values);
  }

  public function buildContent($entity, $view_mode = 'full', $langcode = NULL, $content = array()) {
    $wrapper = entity_metadata_wrapper('example_task', $entity);
    $content['author'] = array('#markup' => t('Created by: !author', array('!author' => $wrapper->uid->name->value(array('sanitize' => TRUE)))));

    // Make Description and Status themed like default fields.
    $content['description'] = array(
      '#theme' => 'field',
      '#weight' => 0,
      '#title' =>t('Description'),
      '#access' => TRUE,
      '#label_display' => 'above',
      '#view_mode' => 'full',
      '#language' => LANGUAGE_NONE,
      '#field_name' => 'field_fake_description',
      '#field_type' => 'text',
      '#entity_type' => 'example_task',
      '#bundle' => $entity->type,
      '#items' => array(array('value' => $entity->description)),
      '#formatter' => 'text_default',
      0 => array('#markup' => check_plain($entity->description))
    );

    return parent::buildContent($entity, $view_mode, $langcode, $content);
  }*/

  /**
   * Implements EntityAPIControllerInterface.
   *
   * @param $transaction
   *   Optionally a DatabaseTransaction object to use. Allows overrides to pass
   *   in their transaction object.
   */
  public function save($entity, DatabaseTransaction $transaction = NULL) {
    $transaction = isset($transaction) ? $transaction : db_transaction();

    try {
      // Load the stored entity, if any.
      if (!empty($entity->{$this->idKey}) && !isset($entity->original)) {
        // In order to properly work in case of name changes, load the original
        // entity using the id key if it is available.
        $entity->original = entity_load_unchanged($this->entityType, $entity->{$this->idKey});
      }

      $entity->is_new = !empty($entity->is_new) || empty($entity->{$this->idKey});
      $entity->updated = REQUEST_TIME;

      $this->invoke('presave', $entity);
      $op = $entity->is_new ? 'insert' : 'update';

      if ($entity->is_new) {
        $return = drupal_write_record($this->entityInfo['base table'], $entity);
        $this->invoke('insert', $entity);

        //field_attach_insert($this->entityType, $entity);
      }
      else {
        $return = drupal_write_record($this->entityInfo['base table'], $entity, $this->idKey);

        $this->resetCache(array($entity->{$this->idKey}));
        $this->invoke('update', $entity);

        field_attach_update($this->entityType, $entity);

        // Update thread access records.
        if (module_exists('harmony_access')) {
          // Update the thread access table for this thread.
          // @see node_save().
          //harmony_access_acquire_grants($thread, FALSE);
        }
      }

      // Update counts elsewhere.
      if ($entity->is_new) {
        harmony_core_update_meta_count(array('field_harmony_thread_count'), 'user', $entity->uid);

        $wrapper = entity_metadata_wrapper('harmony_thread', $entity);

        // Increment the category thread count if the thread belongs to a category.
        if ($wrapper->field_harmony_category->value()) {
          harmony_core_update_meta_count(array('field_harmony_thread_count'), 'taxonomy_term', $wrapper->field_harmony_category->tid->value());
        }
      }

      // Update path alias.
      module_load_include('inc', 'harmony_core', 'includes/harmony_core.pathauto');
      harmony_core_thread_update_alias($entity, $op);

      // Ignore slave server temporarily.
      db_ignore_slave();
      unset($entity->is_new);
      unset($entity->original);

      // Return the entity.
      return $entity;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception($this->entityType, $e);
      throw $e;
    }
  }

    /**
   * Implements EntityAPIControllerInterface.
   *
   * @param $transaction
   *   Optionally a DatabaseTransaction object to use. Allows overrides to pass
   *   in their transaction object.
   *
   * @see node_delete_multiple()
   */
  public function delete($ids, DatabaseTransaction $transaction = NULL) {
    $entities = $ids ? $this->load($ids) : FALSE;
    if (!$entities) {
      // Do nothing, in case invalid or no ids have been passed.
      return;
    }
    // This transaction causes troubles on MySQL, see
    // http://drupal.org/node/1007830. So we deactivate this by default until
    // is shipped in a point release.
    // $transaction = isset($transaction) ? $transaction : db_transaction();

    try {
      $ids = array_keys($entities);

      db_delete($this->entityInfo['base table'])
        ->condition($this->idKey, $ids, 'IN')
        ->execute();

      // Remove thread read history.
      db_delete('harmony_thread_read_history')
        ->condition('thread_id', $ids, 'IN')
        ->execute();

      // Reset the cache as soon as the changes have been applied.
      $this->resetCache($ids);

      foreach ($entities as $id => $entity) {
        $wrapper = entity_metadata_wrapper('harmony_thread', $entity);

        // Decrement some thread counts.
        if ($wrapper->field_harmony_category->value()) {
          harmony_core_update_meta_count(array('field_harmony_thread_count'), 'taxonomy_term', $wrapper->field_harmony_category->tid->value(), 'decrement');
        }

        // User thread count.
        harmony_core_update_meta_count(array('field_harmony_thread_count'), 'user', $entity->uid, 'decrement');

        // Unpublish and delete the posts in the thread.
        // Unpublish.
        $unpublish_query = "UPDATE {harmony_post} AS hp
        INNER JOIN {field_data_field_harmony_thread} as fdfht
        SET hp.status = '0'
        WHERE fdfht.entity_type = 'harmony_post'
        AND fdfht.field_harmony_thread_target_id = :thread_id";
        db_query($unpublish_query, array(':thread_id' => $id));

        // Delete.
        $query = new EntityFieldQuery;
        $query_result = $query
          ->entityCondition('entity_type', 'harmony_post')
          ->fieldCondition('field_harmony_thread', 'target_id', $id)
          ->execute();

        $entity->mass_update_set = FALSE;
        if (!empty($query_result['harmony_post'])) {
          $posts = array_keys($query_result['harmony_post']);
          harmony_core_mass_update('harmony_post', $posts, 'delete');
          $entity->mass_update_set = TRUE;
        }

        // Invoke delete hooks.
        $this->invoke('delete', $entity);
        field_attach_delete($this->entityType, $entity);

        // Return the entity.
        return $entity;
      }
      // Ignore slave server temporarily.
      db_ignore_slave();
    }
    catch (Exception $e) {
      if (isset($transaction)) {
        $transaction->rollback();
      }
      watchdog_exception($this->entityType, $e);
      throw $e;
    }
  }
}

/**
 * Thread class.
 */
class HarmonyThread extends Entity {
  /*protected function defaultLabel() {
    return $this->title;
  }

  protected function defaultUri() {
    return array('path' => 'thread/' . $this->identifier());
  }*/
}

class HarmonyPostController extends EntityAPIController {
  /**
   * Implements EntityAPIControllerInterface.
   *
   * @param $transaction
   *   Optionally a DatabaseTransaction object to use. Allows overrides to pass
   *   in their transaction object.
   */
  public function save($entity, DatabaseTransaction $transaction = NULL) {
    $transaction = isset($transaction) ? $transaction : db_transaction();

    try {
      // Load the stored entity, if any.
      if (!empty($entity->{$this->idKey}) && !isset($entity->original)) {
        // In order to properly work in case of name changes, load the original
        // entity using the id key if it is available.
        $entity->original = entity_load_unchanged($this->entityType, $entity->{$this->idKey});
      }

      $entity->is_new = !empty($entity->is_new) || empty($entity->{$this->idKey});
      $entity->timestamp = REQUEST_TIME;
      $entity->updated = REQUEST_TIME;
      if (empty($entity->log)) {
        $entity->log = '';
      }

      $this->invoke('presave', $entity);

      if ($entity->is_new) {
        $return = drupal_write_record($this->entityInfo['base table'], $entity);
        if ($this->revisionKey) {
          $this->saveRevision($entity);
        }
        $this->invoke('insert', $entity);

        //field_attach_insert($this->entityType, $entity);
      }
      else {
        // Update the base table if the entity doesn't have revisions or
        // we are updating the default revision.
        $entity->is_new_revision = TRUE;
        $entity->default_revision = TRUE;

        if (!$this->revisionKey || !empty($entity->{$this->defaultRevisionKey})) {
          $return = drupal_write_record($this->entityInfo['base table'], $entity, $this->idKey);
        }
        if ($this->revisionKey) {
          $return = $this->saveRevision($entity);
        }

        $this->resetCache(array($entity->{$this->idKey}));
        $this->invoke('update', $entity);
        //field_attach_update($this->entityType, $entity);

        // Field API always saves as default revision, so if the revision saved
        // is not default we have to restore the field values of the default
        // revision now by invoking field_attach_update() once again.
        if ($this->revisionKey && !$entity->{$this->defaultRevisionKey} && !empty($this->entityInfo['fieldable'])) {
          field_attach_update($this->entityType, $entity->original);
        }

        // Update post access records.
        if (module_exists('harmony_access')) {
          // Update the post access table for this post.
          // @see node_save().
          //harmont_access_acquire_grants($post, FALSE);
        }
      }

      // Update some details on the thread.
      $wrapper = entity_metadata_wrapper('harmony_post', $entity);
      if ($wrapper->field_harmony_thread->value()) {
        $thread_id = $wrapper->field_harmony_thread->thread_id->value();
        // Set who last updated the thread.
        $wrapper->field_harmony_thread->field_harmony_last_user_to_reply->set($entity->uid);
        // Set the last time the thread was updated.
        $wrapper->field_harmony_thread->updated->set(REQUEST_TIME);
        $wrapper->field_harmony_thread->save();

        // Update the post count for the thread.
        harmony_core_update_meta_count(array('field_harmony_post_count'), 'harmony_thread', $thread_id);
      }

      // Update post counts elsewhere.
      if ($entity->is_new) {
        harmony_core_update_meta_count(array('field_harmony_post_count'), 'user', $entity->uid);
      }

      // Ignore slave server temporarily.
      db_ignore_slave();
      unset($entity->is_new);
      unset($entity->original);

      // Return the entity.
      return $entity;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception($this->entityType, $e);
      throw $e;
    }
  }

  protected function saveRevision($entity) {
    // Update the reivion user to be the current person editing.
    global $user;
    $entity->uid = $user->uid;

    // Convert the entity into an array as it might not have the same properties
    // as the entity, it is just a raw structure.
    $record = (array) $entity;
    // File fields assumes we are using $entity->revision instead of
    // $entity->is_new_revision, so we also support it and make sure it's set to
    // the same value.
    $entity->is_new_revision = !empty($entity->is_new_revision) || !empty($entity->revision) || $entity->is_new;
    $entity->revision = &$entity->is_new_revision;
    $entity->{$this->defaultRevisionKey} = !empty($entity->{$this->defaultRevisionKey}) || $entity->is_new;

    // When saving a new revision, set any existing revision ID to NULL so as to
    // ensure that a new revision will actually be created.
    if ($entity->is_new_revision && isset($record[$this->revisionKey])) {
      $record[$this->revisionKey] = NULL;
    }

    if ($entity->is_new_revision) {
      drupal_write_record($this->revisionTable, $record);
      $update_default_revision = $entity->{$this->defaultRevisionKey};
    }
    else {
      drupal_write_record($this->revisionTable, $record, $this->revisionKey);
      // @todo: Fix original entity to be of the same revision and check whether
      // the default revision key has been set.
      $update_default_revision = $entity->{$this->defaultRevisionKey} && $entity->{$this->revisionKey} != $entity->original->{$this->revisionKey};
    }
    // Make sure to update the new revision key for the entity.
    $entity->{$this->revisionKey} = $record[$this->revisionKey];

    // Mark this revision as the default one.
    if ($update_default_revision) {
      db_update($this->entityInfo['base table'])
        ->fields(array($this->revisionKey => $record[$this->revisionKey]))
        ->condition($this->idKey, $entity->{$this->idKey})
        ->execute();
    }
    return $entity->is_new_revision ? SAVED_NEW : SAVED_UPDATED;
  }

  public function buildQuery($ids, $conditions = array(), $revision_id = FALSE) {
    // Add an alias to this query to ensure that we can tell if this is
    // the current revision or not.
    $query = parent::buildQuery($ids, $conditions, $revision_id);
    $query->addField('base', 'vid', 'current_vid');

    return $query;
  }

   /**
   * Implements EntityAPIControllerInterface.
   *
   * @param $transaction
   *   Optionally a DatabaseTransaction object to use. Allows overrides to pass
   *   in their transaction object.
   *
   * @see node_delete_multiple()
   */
  public function delete($ids, DatabaseTransaction $transaction = NULL) {
    $entities = $ids ? $this->load($ids) : FALSE;
    if (!$entities) {
      // Do nothing, in case invalid or no ids have been passed.
      return;
    }
    // This transaction causes troubles on MySQL, see
    // http://drupal.org/node/1007830. So we deactivate this by default until
    // is shipped in a point release.
    // $transaction = isset($transaction) ? $transaction : db_transaction();

    try {
      $ids = array_keys($entities);

      // Prevent any first posts in a thread getting deleted.
      foreach ($ids as $k => $entity_id) {
        if ($entities[$entity_id]->first_post) {
          unset($ids[$k]);
          unset($entities[$entity_id]);
        }
      }

      db_delete($this->entityInfo['base table'])
        ->condition($this->idKey, $ids, 'IN')
        ->execute();

      if (isset($this->revisionTable)) {
        db_delete($this->revisionTable)
          ->condition($this->idKey, $ids, 'IN')
          ->execute();
      }

      // Reset the cache as soon as the changes have been applied.
      $this->resetCache($ids);

      foreach ($entities as $id => $entity) {
        $wrapper = entity_metadata_wrapper('harmony_post', $entity);
        $thread_id = NULL;
        if ($wrapper->field_harmony_thread->value()) {
          $thread_id = $wrapper->field_harmony_thread->thread_id->value();
        }

        // Update the post count on the thread.
        if ($thread_id) {
          harmony_core_update_meta_count(array('field_harmony_post_count'), 'harmony_thread', $thread_id, 'decrement');
        }

        // Update post count for the user.
        harmony_core_update_meta_count(array('field_harmony_post_count'), 'user', $entity->uid, 'decrement');

        // Remove the Likes from the thread associated with this post.
        if ($thread_id) {
          $likes = flag_get_counts('harmony_post', $id);
          if (!empty($likes['harmony_likes'])) {
            $post_likes = $likes['harmony_likes'];
            $thread_likes = $wrapper->field_harmony_thread->likes->value();
            $wrapper->field_harmony_thread->likes->set($thread_likes - $post_likes);
            $wrapper->field_harmony_thread->save();
          }
        }

        $this->invoke('delete', $entity);
        field_attach_delete($this->entityType, $entity);
      }
      // Ignore slave server temporarily.
      db_ignore_slave();
    }
    catch (Exception $e) {
      if (isset($transaction)) {
        $transaction->rollback();
      }
      watchdog_exception($this->entityType, $e);
      throw $e;
    }
  }

  public function deleteRevision($revision_id, DatabaseTransaction $transaction = NULL) {
    $revision = reset(harmony_post_load_multiple(array(), array('vid' => $revision_id)));

    if (!$revision) {
      // Do nothing, in case invalid or no id has been passed.
      return FALSE;
    }

    try {
      // Prevent deleting the current revision.
      $post = harmony_post_load($revision->post_id);
      if ($revision_id == $post->vid) {
        return FALSE;
      }

      db_delete($this->revisionTable)
        ->condition($this->idKey, $revision->post_id)
        ->condition('vid', $revision->vid)
        ->execute();
      module_invoke_all('harmony_post_revision_delete', $revision);
      field_attach_delete_revision($this->entityType, $revision);

      // Reset the cache as soon as the changes have been applied.
      $this->resetCache(array($revision->post_id));

      // Ignore slave server temporarily.
      db_ignore_slave();

      return TRUE;
    }
    catch (Exception $e) {
      if (isset($transaction)) {
        $transaction->rollback();
      }
      watchdog_exception($this->entityType, $e);
      throw $e;
    }

    return FALSE;
  }
}

/**
 * Post class.
 */
class HarmonyPost extends Entity {
}
