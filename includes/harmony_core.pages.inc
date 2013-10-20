<?php

/**
 * @file
 * harmony_core.pages.inc
 * Page callbacks for adding, editing and deleting threads and posts.
 */

function harmony_core_thread_add() {
  global $user;

  $thread = (object) array(
    'title' => '',
    'uid' => $user->uid,
    'language' => LANGUAGE_NONE,
    'status' => HARMONY_PUBLISHED,
    'created' => REQUEST_TIME,
    'updated' => REQUEST_TIME,
    'pinned' => THREAD_NOT_PINNED,
    'locked' => HARMONY_NOT_LOCKED,
    'is_new' => TRUE,
  );
  
  drupal_set_title(t('Create thread'), PASS_THROUGH);
  $output = drupal_get_form('harmony_core_thread_form', $thread);

  return $output;
}

function harmony_core_thread_view($thread, $view_mode = 'full') {
  $uri = entity_uri('thread', $thread);
  // Set the thread path as the canonical URL to prevent duplicate content.
  drupal_add_html_head_link(array('rel' => 'canonical', 'href' => url($uri['path'], $uri['options'])), TRUE);
  // Set the non-aliased path as a default shortlink.
  drupal_add_html_head_link(array('rel' => 'shortlink', 'href' => url($uri['path'], array_merge($uri['options'], array('alias' => TRUE)))), TRUE);

  // Trigger context if viewing a thread page.
  if ($view_mode == 'full') {
    if ($plugin = context_get_plugin('condition', 'thread')) {
      $plugin->execute($thread);
    }
  }

  // Update last read date for this thread and user.
  harmony_core_thread_last_read_set($thread);

  dsm($thread);

  return '';
}

/**
 * Form submission handler for harmony_core_thread_form().
 *
 * Handles the 'Delete' button on the thread form.
 *
 * @see harmony_core_thread_form()
 */
function harmony_core_delete_thread_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $thread = $form['thread']['#value'];
  $form_state['redirect'] = array('thread/' . $thread->thread_id . '/delete', array('query' => $destination));
}

function harmony_core_delete_thread_confirm($form, &$form_state, $thread) {
  $form['#thread'] = $thread;

  // Always provide entity id in the same form key as in the entity edit form.
  $form['thread_id'] = array('#type' => 'value', '#value' => $thread->thread_id);

  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $thread->title)),
    'thread/' . $thread->thread_id,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Executes thread deletion.
 *
 * @see harmony_core_delete_thread_confirm()
 */
function harmony_core_delete_thread_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $thread = harmony_core_thread_load($form_state['values']['thread_id']);
    harmony_core_delete_thread($thread->thread_id);

    watchdog('harmony', 'Thread: deleted %title.', array('%title' => $thread->title));
    drupal_set_message(t('The thread %title has been deleted.', array('%title' => $thread->title)));
  }

  $form_state['redirect'] = '<front>';
}

function harmony_core_post_add() {
  global $user;

  $post = (object) array(
    'title' => '',
    'uid' => $user->uid,
    'language' => LANGUAGE_NONE,
    'status' => HARMONY_PUBLISHED,
    'created' => REQUEST_TIME,
    'updated' => REQUEST_TIME,
    'locked' => HARMONY_NOT_LOCKED,
    'is_new' => TRUE,
  );
  
  drupal_set_title(t('Create post'), PASS_THROUGH);
  $output = drupal_get_form('harmony_core_post_form', $post);

  return $output;
}

function harmony_core_post_view($post, $view_mode = 'full') {
  dsm($post);

  return '';
}

/**
 * Form submission handler for harmony_core_post_form().
 *
 * Handles the 'Delete' button on the post form.
 *
 * @see harmony_core_post_form()
 */
function harmony_core_delete_post_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $post = $form['post']['#value'];
  $form_state['redirect'] = array('post/' . $post->post_id . '/delete', array('query' => $destination));
}

function harmony_core_delete_post_confirm($form, &$form_state, $post) {
  $form['#post'] = $post;

  // Always provide entity id in the same form key as in the entity edit form.
  $form['post_id'] = array('#type' => 'value', '#value' => $post->post_id);

  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $post->title)),
    'post/' . $post->post_id,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Executes post deletion.
 *
 * @see harmony_core_delete_post_confirm()
 */
function harmony_core_delete_post_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $post = harmony_core_post_load($form_state['values']['post_id']);
    harmony_core_delete_post($post->post_id);

    watchdog('harmony', 'Post: deleted %title.', array('%title' => $post->title));
    drupal_set_message(t('The post %title has been deleted.', array('%title' => $post->title)));
  }

  $form_state['redirect'] = '<front>';
}
