<?php

/**
 * @file
 * Class definitions for the Thread and Post entity.
 */

class HarmonyThreadController extends EntityAPIController {
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
      }
      else {
        $return = drupal_write_record($this->entityInfo['base table'], $entity, $this->idKey);

        $this->resetCache(array($entity->{$this->idKey}));
        $this->invoke('update', $entity);

        field_attach_update($this->entityType, $entity);
      }

      // Update thread access records.
      if (module_exists('harmony_access')) {
        harmony_access_acquire_records($entity);
      }

      $entity->batch_process = FALSE;

      // Hold on to your bootstraps! Lets update some counts.
      // Only act if either we're dealing with a new entity, or if the
      // published status has changed on an existing entity.
      $count_direction = NULL;
      if ($entity->is_new && $entity->status == HARMONY_PUBLISHED || !$entity->is_new && $entity->status != $entity->original->status) {
        // Determine the direction.
        // Increment will be a new entity or an entity that was previously
        // unpublished and is now published.
        if (
          $entity->is_new && $entity->status == HARMONY_PUBLISHED ||
          !$entity->is_new && $entity->status == HARMONY_PUBLISHED && $entity->original->status == HARMONY_NOT_PUBLISHED
        ) {
          $count_direction = 'increment';
        }
        // Decrement, a pre-existing entity that's become unpublished.
        // Pre-existing only as if it hasn't existed before, it won't
        // have effected the count.
        elseif (!$entity->is_new && $entity->status == HARMONY_NOT_PUBLISHED && $entity->original->status == HARMONY_PUBLISHED) {
          $count_direction = 'decrement';
        }

        // Thread counts, act if we're incrementing or decrementing, it's
        // possible that we won't do anything, in which case nothing
        // should change.
        // In the case where a thread is being unpublished, it's posts will
        // be too. We don't do anything as on unpublish of each post the
        // users post count will be decremented, so we only do the thread
        // count here.
        if ($count_direction && $entity->uid > 0) {
          harmony_core_update_meta_count(array('field_harmony_thread_count'), 'user', $entity->uid, $count_direction);
        }

        $wrapper = entity_metadata_wrapper('harmony_thread', $entity);

        // Increment the category thread count if the thread belongs to a category.
        if ($count_direction && $wrapper->field_harmony_category->value()) {
          harmony_core_update_meta_count(array('field_harmony_thread_count'), 'taxonomy_term', $wrapper->field_harmony_category->tid->value(), $count_direction);

          // Also update the post count for categories if not new, as new threads
          // will have a post count of 0.
          if (!$entity->is_new) {
            harmony_core_update_meta_count(array('field_harmony_post_count'), 'taxonomy_term', $wrapper->field_harmony_category->tid->value(), $count_direction, $wrapper->field_harmony_post_count->value());
          }
        }
      }

      // Lot's of things should flow down from the thread to its posts,
      // this is where we compile the things that have changed and will
      // be applied en masse.
      $post_properties_to_change = array();
      // If a thread title is changed, so should its posts.
      if (!$entity->is_new && $entity->title !== $entity->original->title) {
        $post_properties_to_change['title'] = $entity->title;
      }

      // Newly locked threads should propogate through to its posts, batch!
      if (!$entity->is_new && $entity->locked == HARMONY_LOCKED && $entity->original->locked == HARMONY_NOT_LOCKED) {
        $post_properties_to_change['locked'] = HARMONY_LOCKED;
      }
      // And unlock.
      elseif (!$entity->is_new && $entity->locked == HARMONY_NOT_LOCKED && $entity->original->locked == HARMONY_LOCKED) {
        $post_properties_to_change['locked'] = HARMONY_NOT_LOCKED;
      }

      // If a thread becomes unpublished then we hide the posts from the site.
      // This means that post counts can remain the same for threads
      if (!$entity->is_new && $entity->status == HARMONY_NOT_PUBLISHED && $entity->original->status == HARMONY_PUBLISHED) {
        $post_properties_to_change['hidden'] = HARMONY_HIDDEN;
      }
      // Thread is now published, publish posts.
      elseif (!$entity->is_new && $entity->status == HARMONY_PUBLISHED && $entity->original->status == HARMONY_NOT_PUBLISHED) {
        $post_properties_to_change['hidden'] = HARMONY_NOT_HIDDEN;
      }

      // If changes are desired for a threads posts, do it!
      if (!empty($post_properties_to_change)) {
        $query = new EntityFieldQuery;
        $query_result = $query
          ->entityCondition('entity_type', 'harmony_post')
          ->fieldCondition('field_harmony_thread', 'target_id', $entity->{$this->idKey})
          ->execute();

        if (!empty($query_result['harmony_post'])) {
          $posts = array_keys($query_result['harmony_post']);
          harmony_core_mass_update('harmony_post', $posts, 'update', $post_properties_to_change);

          if (count($posts) > variable_get('harmony_core_mass_update_batch_threshold', 10)) {
            $entity->batch_process = TRUE;
          }
        }
      }

      // Update path alias for the thread.
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
        if ($entity->status == HARMONY_PUBLISHED && $wrapper->field_harmony_category->value()) {
          harmony_core_update_meta_count(array('field_harmony_thread_count'), 'taxonomy_term', $wrapper->field_harmony_category->tid->value(), 'decrement');

          // Post counts for taxonomy terms as the link will be severed.
          if ($wrapper->field_harmony_post_count->value() > 0) {
            harmony_core_update_meta_count(array('field_harmony_post_count'), 'taxonomy_term', $wrapper->field_harmony_category->tid->value(), 'decrement', $wrapper->field_harmony_post_count->value());
          }
        }

        // User thread count.
        if ($entity->status == HARMONY_PUBLISHED && $entity->uid > 0) {
          harmony_core_update_meta_count(array('field_harmony_thread_count'), 'user', $entity->uid, 'decrement');
        }

        // Delete.
        $query = new EntityFieldQuery;
        $query_result = $query
          ->entityCondition('entity_type', 'harmony_post')
          ->fieldCondition('field_harmony_thread', 'target_id', $id)
          ->execute();

        $entity->batch_process = FALSE;
        if (!empty($query_result['harmony_post'])) {
          $posts = array_keys($query_result['harmony_post']);
          harmony_core_mass_update('harmony_post', $posts, 'delete');

          if (count($posts) > variable_get('harmony_core_mass_update_batch_threshold', 10)) {
            $entity->batch_process = TRUE;
          }
        }

        // Invoke delete hooks.
        $this->invoke('delete', $entity);
        field_attach_delete($this->entityType, $entity);

        // Delete path alias.
        pathauto_entity_path_delete_all('harmony_thread', $entity, "thread/{$entity->thread_id}");

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
      $wrapper = entity_metadata_wrapper('harmony_post', $entity);

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
        $entity->default_revision = TRUE;

        // Only save a new revision if certain fields have been changed.
        $entity->is_new_revision = FALSE;
        if (!empty($entity->original)) {
          $original_post_wrapper = entity_metadata_wrapper('harmony_post', $entity->original);

          $trigger_fields = variable_get('harmony_core_post_revision_tigger_fields', array('field_harmony_text' => 'value'));

          if (!empty($trigger_fields) && is_array($trigger_fields)) {
            foreach ($trigger_fields as $field => $index) {
              $original = $original_post_wrapper->{$field}->value();
              $current = $wrapper->{$field}->value();

              // If a specific index is specified use it, otherwise assume
              // that the returned data should be compared.
              if ($index != '' && isset($original[$index])) {
                $original = $original[$index];
                $current = $current[$index];
              }

              if ($original != $current) {
                $entity->is_new_revision = TRUE;
                break;
              }
            }
          }
          else {
            watchdog('harmony_core', "Somehow an invalid value has gotten into the variable %var_name. This will prevent harmony_core from figuring out when new post revisions shouldn't be saved.", array('%var_name' => 'harmony_core_post_revision_tigger_fields'));
          }
        }

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
          //harmony_access_acquire_grants($post, FALSE);
        }
      }

      // Update some details on the thread.
      if ($wrapper->field_harmony_thread->value()) {
        $thread_id = $wrapper->field_harmony_thread->thread_id->value();
      }
      else {
        $thread_id = NULL;
      }

      // If we're attached to a thread, update some things!
      if ($thread_id) {
        // Set who last updated the thread.
        if ($entity->uid > 0) {
          $wrapper->field_harmony_thread->field_harmony_last_user_to_reply->set($entity->uid);
        }
        // Set the last time the thread was updated.
        $wrapper->field_harmony_thread->updated->set(REQUEST_TIME);
        $wrapper->field_harmony_thread->save();
      }

      // Update counts.
      $count_direction = NULL;
      // The hidden flag has a different purpose for posts than it does threads.
      // A post becomes hidden because it's parent thread is unpublished, using
      // the hidden flag allows the thread to retain it's post count (which it
      // should as the posts are still technically active). This is the only
      // time when hidden is used, it is not exposed on any forms.
      $hidden_changed = !$entity->is_new && $entity->hidden != $entity->original->hidden;
      if ($entity->is_new && $entity->status == HARMONY_PUBLISHED || !$entity->is_new && $entity->status != $entity->original->status || $hidden_changed) {
        // Determine the direction.
        if (
          $entity->is_new && $entity->status == HARMONY_PUBLISHED ||
          !$entity->is_new && $entity->status == HARMONY_PUBLISHED && $entity->original->status == HARMONY_NOT_PUBLISHED ||
          !$entity->is_new && $entity->hidden == HARMONY_NOT_HIDDEN && $entity->original->hidden == HARMONY_HIDDEN
        ) {
          $count_direction = 'increment';
        }
        elseif (
          !$entity->is_new && $entity->status == HARMONY_NOT_PUBLISHED && $entity->original->status == HARMONY_PUBLISHED ||
          !$entity->is_new && $entity->status == HARMONY_HIDDEN && $entity->original->status == HARMONY_NOT_HIDDEN
        ) {
          $count_direction = 'decrement';
        }

        // User count.
        if ($count_direction) {
          // Thread count.
          if ($thread_id && !$hidden_changed) {
            harmony_core_update_meta_count(array('field_harmony_post_count'), 'harmony_thread', $thread_id, $count_direction);
          }

          // Users post count.
          if ($entity->uid > 0) {
            harmony_core_update_meta_count(array('field_harmony_post_count'), 'user', $entity->uid, $count_direction);
          }

          // Increment the category thread count if the thread belongs to a category.
          // @todo this makes stuff blow up for some reason.
          /*if ($wrapper->field_harmony_thread->field_harmony_category->value()) {
            $term_id = $wrapper->field_harmony_thread->field_harmony_category->tid->value();
            harmony_core_update_meta_count(array('field_harmony_post_count'), 'taxonomy_term', $term_id, $count_direction);
          }*/
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
      watchdog_exception($this->entityType, $e);
      throw $e;
    }
  }

  protected function saveRevision($entity) {
    // Update the reivion user to be the current person editing if not anon.
    // We don't allow anon to create threads or posts but things can happen
    // via drush.
    global $user;
    $entity->uid = $user->uid > 0 ? $user->uid : $entity->uid;

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
          $first_post_wrapper = entity_metadata_wrapper('harmony_post', $entities[$entity_id]);

          // Attempt to load the thread, if we can then do not allow
          // this node to be deleted.
          if (!$first_post_wrapper->field_harmony_thread->value()) {
            continue;
          }
          else {
            $thread_id = $first_post_wrapper->field_harmony_thread->thread_id->value();

            if (harmony_thread_load($thread_id)) {
              unset($ids[$k]);
              unset($entities[$entity_id]);
            }
          }
        }
      }

      // As we remove the first post, check the array isn't empty.
      if (!empty($ids)) {
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

            // Update the post count if the thread is in a category.
            if ($wrapper->field_harmony_thread->field_harmony_category->value()) {
              $term_id = $wrapper->field_harmony_thread->field_harmony_category->tid->value();
              harmony_core_update_meta_count(array('field_harmony_post_count'), 'taxonomy_term', $term_id, 'decrement');
            }
          }

          // Update post count for the user.
          if ($entity->uid > 0) {
            harmony_core_update_meta_count(array('field_harmony_post_count'), 'user', $entity->uid, 'decrement');
          }

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

/**
 * Who knows why but somethin in Entity API or Field API is misaligned
 * so that the manage fields & display paths aren't in the right place.
 * To correct this override the UI controller to change where the links
 * are output to correct the paths. Chances are something in the
 * hook_entity_info is out.
 */
class HarmonyThreadTypeUIController extends EntityDefaultUIController {
  /**
   * Generates the row for the passed entity and may be overridden in order to
   * customize the rows.
   *
   * @param $additional_cols
   *   Additional columns to be added after the entity label column.
   */
  protected function overviewTableRow($conditions, $id, $entity, $additional_cols = array()) {
    $entity_uri = entity_uri($this->entityType, $entity);

    $row[] = array('data' => array(
      '#theme' => 'entity_ui_overview_item',
      '#label' => entity_label($this->entityType, $entity),
      '#name' => !empty($this->entityInfo['exportable']) ? entity_id($this->entityType, $entity) : FALSE,
      '#url' => $entity_uri ? $entity_uri : FALSE,
      '#entity_type' => $this->entityType),
    );

    // Add in any passed additional cols.
    foreach ($additional_cols as $col) {
      $row[] = $col;
    }

    // Add a row for the exportable status.
    if (!empty($this->entityInfo['exportable'])) {
      $row[] = array('data' => array(
        '#theme' => 'entity_status',
        '#status' => $entity->{$this->statusKey},
      ));
    }
    // In case this is a bundle, we add links to the field ui tabs.
    $field_ui = !empty($this->entityInfo['bundle of']) && entity_type_is_fieldable($this->entityInfo['bundle of']) && module_exists('field_ui');
    // For exportable entities we add an export link.
    $exportable = !empty($this->entityInfo['exportable']);
    // If i18n integration is enabled, add a link to the translate tab.
    $i18n = !empty($this->entityInfo['i18n controller class']);

    // Add operations depending on the status.
    if (entity_has_status($this->entityType, $entity, ENTITY_FIXED)) {
      $row[] = array('data' => l(t('clone'), $this->path . '/manage/' . $id . '/clone'), 'colspan' => $this->operationCount());
    }
    else {
      $row[] = l(t('edit'), $this->path . '/manage/' . $id);

      if ($field_ui) {
        $row[] = l(t('manage fields'), $this->path . '/' . $id . '/fields');
        $row[] = l(t('manage display'), $this->path . '/' . $id . '/display');
      }
      if ($i18n) {
        $row[] = l(t('translate'), $this->path . '/manage/' . $id . '/translate');
      }
      if ($exportable) {
        $row[] = l(t('clone'), $this->path . '/manage/' . $id . '/clone');
      }

      if (empty($this->entityInfo['exportable']) || !entity_has_status($this->entityType, $entity, ENTITY_IN_CODE)) {
        if ($id != 'harmony_thread') {
          $row[] = l(t('delete'), $this->path . '/manage/' . $id . '/delete', array('query' => drupal_get_destination()));
        }
        else {
          $row[] = '';
        }
      }
      elseif (entity_has_status($this->entityType, $entity, ENTITY_OVERRIDDEN)) {
        $row[] = l(t('revert'), $this->path . '/manage/' . $id . '/revert', array('query' => drupal_get_destination()));
      }
      else {
        $row[] = '';
      }
    }
    if ($exportable) {
      $row[] = l(t('export'), $this->path . '/manage/' . $id . '/export');
    }
    return $row;
  }
}
