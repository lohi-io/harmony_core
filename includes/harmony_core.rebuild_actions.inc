<?php

/**
 * @file
 * harmony_core.rebuild_actions.inc
 *
 * Operation and Finished callbacks for the rebuild batch actions.
 */

/**
 * Generalised batch rebuild finished function to output standard messages.
 */
function _harmony_core_rebuild_finished($success, $results, $operations, $action_id) {
  $actions = harmony_core_rebuild_actions();
  $action = $actions[$action_id];

  if ($success) {
    drupal_set_message(t('Rebuild of the @title has been completed.', array('@title' => $action['title'])));
    $actions = variable_get('harmony_core_rebuild_actions', array());
    if (($key = array_search($action_id, $actions)) !== FALSE) {
      unset($actions[$key]);
    }
    variable_set('harmony_core_rebuild_actions', $actions);
  }
  else {
    drupal_set_message(t("Something's gone wrong with the @title rebuild, please check the error log.", array('@title' => $action['title'])), 'error');
  }
}

/**
 * Operation callback which batch rebuilds the Last post URI property for
 * Thread entities.
 */
function _harmony_core_rebuild_last_post_uris_operation(&$context) {
  if (empty($context['sandbox'])) {
    // Initiate multistep processing.
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_entity_id'] = 0;
    $context['sandbox']['max'] = db_query('SELECT COUNT(DISTINCT thread_id) FROM {harmony_thread}')->fetchField();
  }

  // Process the next 20 threads.
  $limit = 20;
  $thread_ids = db_query_range("SELECT thread_id FROM {harmony_thread} WHERE thread_id > :entity_id ORDER BY thread_id ASC", 0, $limit, array(':entity_id' => $context['sandbox']['current_entity_id']))->fetchCol();
  $threads = harmony_thread_load_multiple($thread_ids, array(), TRUE);

  foreach ($threads as $thread_id => $thread) {
    if (!empty($thread)) {
      $thread_posts_stored = array();
      $thread_posts = harmony_core_get_thread_posts($thread->thread_id);

      $thread_posts_stored['all'] = $thread_posts;
      $thread_posts_stored['first'] = array_shift($thread_posts);
      $thread_posts_stored['last'] = !empty($thread_posts) ? array_pop($thread_posts) : $thread_posts_stored['first'];
      $thread_posts_stored['other'] = $thread_posts;

      $last_post = harmony_post_load($thread_posts_stored['last']);
      $thread->last_post_uri = harmony_core_post_thread_url($thread->thread_id, $last_post->post_id, TRUE);
      harmony_thread_meta_save($thread);
    }

    $context['sandbox']['progress']++;
    $context['sandbox']['current_entity_id'] = $thread_id;
  }

  // Multistep processing : report progress.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Finished callback for the Last post URI rebuild action.
 */
function _harmony_core_rebuild_last_post_uris_operation_finished($success, $results, $operations) {
  _harmony_core_rebuild_finished($success, $results, $operations, 'last_post_uris');
}
