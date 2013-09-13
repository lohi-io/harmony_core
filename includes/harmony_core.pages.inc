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
    'status' => THREAD_PUBLISHED,
    'created' => REQUEST_TIME,
    'updated' => REQUEST_TIME,
    'pinned' => THREAD_NOT_PINNED,
    'locked' => THREAD_NOT_LOCKED,
    'is_new' => TRUE,
  );
  
  drupal_set_title(t('Create thread'), PASS_THROUGH);
  $output = drupal_get_form('harmony_core_thread_form', $thread);

  return $output;
}

function harmony_core_thread_view($thread, $view_mode = 'full') {
  // Trigger context if viewing a thread page.
  if ($view_mode == 'full') {
    if ($plugin = context_get_plugin('condition', 'thread')) {
      $plugin->execute($thread);
    }
  }

  return '';
}

/**
 * Form submission handler for harmony_core_thread_form().
 *
 * Handles the 'Delete' button on the thread form.
 *
 * @see harmony_core_thread_form()
 */
function harmony_core_thread_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $thread = $form['thread']['#value'];
  $form_state['redirect'] = array('thread/' . $thread->thread_id . '/delete', array('query' => $destination));
}

function harmony_core_thread_delete_confirm($form, &$form_state, $thread) {
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
 * @see harmony_core_thread_delete_confirm()
 */
function harmony_core_thread_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $thread = harmony_core_thread_load($form_state['values']['thread_id']);
    harmony_core_thread_delete($thread);

    watchdog('harmony', 'Thread: deleted %title.', array('%title' => $thread->title));
    drupal_set_message(t('The thread %title has been deleted.', array('%title' => $thread->title)));
  }

  $form_state['redirect'] = '<front>';
}
