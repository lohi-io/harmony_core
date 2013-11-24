<?php

/**
 * @file
 * Class definitions for the Thread and Post entity.
 */

class ThreadController extends EntityAPIController {

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

      // Ignore slave server temporarily.
      db_ignore_slave();
      unset($entity->is_new);
      unset($entity->original);

      // Return the entity.
      return $entity;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('thread', $e);
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
      db_delete('thread_read_history')
        ->condition('thread_id', $ids, 'IN')
        ->execute();

      // Reset the cache as soon as the changes have been applied.
      $this->resetCache($ids);

      foreach ($entities as $id => $entity) {
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
}

/**
 * Thread class.
 */
class Thread extends Entity {
  /*protected function defaultLabel() {
    return $this->title;
  }

  protected function defaultUri() {
    return array('path' => 'thread/' . $this->identifier());
  }*/
}

class PostController extends EntityAPIController {
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

      // Ignore slave server temporarily.
      db_ignore_slave();
      unset($entity->is_new);
      unset($entity->original);

      // Return the entity.
      return $entity;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('post', $e);
      throw $e;
    }
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
class Post extends Entity {
}
