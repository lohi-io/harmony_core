<?php

/**
 * @file
 * harmony_core.pages.inc
 * Page callbacks for adding, editing and deleting threads and posts.
 */

function harmony_core_thread_add($type) {
  global $user;

  // Determine thread type.
  if ($type == '') {
    $type = 'harmony-thread';
  }
  $type = str_replace('-', '_', $type);
  $thread_types = harmony_core_get_thread_types();
  if (!in_array($type, array_keys($thread_types))) {
    drupal_not_found();
  }

  $thread = (object) array(
    'title' => '',
    'type' => $type,
    'uid' => $user->uid,
    'language' => LANGUAGE_NONE,
    'status' => HARMONY_PUBLISHED,
    'created' => REQUEST_TIME,
    'updated' => REQUEST_TIME,
    'pinned' => HARMONY_NOT_PINNED,
    'locked' => HARMONY_NOT_LOCKED,
    'hidden' => HARMONY_NOT_HIDDEN,
    'is_new' => TRUE,
  );

  drupal_set_title(t('Create thread'), PASS_THROUGH);
  $output = drupal_get_form('harmony_core_thread_form', $thread);

  return $output;
}

function harmony_core_thread_view_page($thread, $view_mode = 'full', $langcode = NULL) {
  $uri = entity_uri('harmony_thread', $thread);
  // Set the thread path as the canonical URL to prevent duplicate content.
  drupal_add_html_head_link(array('rel' => 'canonical', 'href' => url($uri['path'], $uri['options'])), TRUE);
  // Set the non-aliased path as a default shortlink.
  drupal_add_html_head_link(array('rel' => 'shortlink', 'href' => url($uri['path'], array_merge($uri['options'], array('alias' => TRUE)))), TRUE);

  // Trigger context if viewing a thread page.
  if ($view_mode == 'full') {
    if ($plugin = context_get_plugin('condition', 'harmony_thread')) {
      $plugin->execute($thread);
    }
  }

  // Update last read date for this thread and user.
  harmony_core_thread_last_read_set($thread);

  return harmony_core_thread_view_multiple(array($thread->thread_id => $thread), $view_mode, $langcode);
}

function harmony_core_thread_view_multiple($threads, $view_mode = 'full', $langcode = NULL) {
  field_attach_prepare_view('harmony_thread', $threads, $view_mode, $langcode);
  entity_prepare_view('harmony_thread', $threads, $langcode);

  $build = array();
  foreach ($threads as $thread) {
    $build['threads'][$thread->thread_id] = harmony_core_thread_view($thread, $view_mode, $langcode);
  }

  return $build;
}

function harmony_core_thread_view($thread, $view_mode = 'full', $langcode = NULL) {
  if (!isset($langcode)) {
    $langcode = $GLOBALS['language_content']->language;
  }

  // Populate $thread->content with a render() array.
  harmony_core_thread_build_content($thread, $view_mode, $langcode);

  $build = $thread->content;
  // We don't need duplicate rendering info in thread->content.
  unset($thread->content);

  $build += array(
    '#theme' => 'harmony_thread',
    '#thread' => $thread,
    '#view_mode' => $view_mode,
    '#language' => $langcode,
  );

  // Allow modules to modify the structured thread.
  $type = 'harmony_thread';
  drupal_alter(array('harmony_thread_view', 'entity_view'), $build, $type);

  return $build;
}

function harmony_core_thread_build_content($thread, $view_mode = 'full', $langcode = NULL) {
  if (!isset($langcode)) {
    $langcode = $GLOBALS['language_content']->language;
  }

  // Remove previously built content, if exists.
  $thread->content = array();

  // Allow modules to change the view mode.
  $context = array(
    'entity_type' => 'harmony_thread',
    'entity' => $thread,
    'langcode' => $langcode,
  );
  drupal_alter('entity_view_mode', $view_mode, $context);

  // Build fields content.
  // In case of a multiple view, harmony_core_thread_view_multiple() already ran the
  // 'prepare_view' step. An internal flag prevents the operation from running
  // twice.
  field_attach_prepare_view('harmony_thread', array($thread->thread_id => $thread), $view_mode, $langcode);
  entity_prepare_view('harmony_thread', array($thread->thread_id => $thread), $langcode);
  $thread->content += field_attach_view('harmony_thread', $thread, $view_mode, $langcode);

  // Allow modules to make their own additions to the thread.
  module_invoke_all('harmony_thread_view', $thread, $view_mode, $langcode);
  module_invoke_all('entity_view', $thread, 'harmony_thread', $view_mode, $langcode);

  // Make sure the current view mode is stored if no module has already
  // populated the related key.
  $thread->content += array('#view_mode' => $view_mode);
}

/**
 * Form submission handler for harmony_core_thread_form().
 *
 * Handles the 'Delete' button on the thread form.
 *
 * @see harmony_core_thread_form()
 */
function harmony_thread_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $thread = $form['thread']['#value'];
  $form_state['redirect'] = array('thread/' . $thread->thread_id . '/delete', array('query' => $destination));
}

function harmony_thread_delete_confirm($form, &$form_state, $thread) {
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
 * @see harmony_thread_delete_confirm()
 */
function harmony_thread_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $thread = harmony_thread_load($form_state['values']['thread_id']);
    $deleted_thread = harmony_thread_delete($thread->thread_id);

    if (!empty($deleted_thread->batch_process) && $deleted_thread->batch_process) {
      // The delete process can result in batch operations for deleting
      // a posts thread, process!
      batch_process('<front>');
    }

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
    'hidden' => HARMONY_NOT_HIDDEN,
    'is_new' => TRUE,
  );

  drupal_set_title(t('Create post'), PASS_THROUGH);
  $output = drupal_get_form('harmony_core_post_form', $post);

  return $output;
}

function harmony_core_post_view_page($post, $view_mode = 'full', $langcode = NULL) {
  return harmony_core_post_view_multiple(array($post->post_id => $post), $view_mode, $langcode);
}

function harmony_core_post_view_multiple($posts, $view_mode = 'full', $langcode = NULL) {
  field_attach_prepare_view('harmony_post', $posts, $view_mode, $langcode);
  entity_prepare_view('harmony_post', $posts, $langcode);

  $build = array();
  foreach ($posts as $post) {
    $build['posts'][$post->post_id] = harmony_core_post_view($post, $view_mode, $langcode);
  }

  return $build;
}

function harmony_core_post_view($post, $view_mode = 'full', $langcode = NULL) {
  if (!isset($langcode)) {
    $langcode = $GLOBALS['language_content']->language;
  }

  // Populate $post->content with a render() array.
  harmony_core_post_build_content($post, $view_mode, $langcode);

  $build = $post->content;
  // We don't need duplicate rendering info in post->content.
  unset($post->content);

  // Add more to the build object.
  $build += array(
    '#theme' => 'harmony_post',
    '#post' => $post,
    '#view_mode' => $view_mode,
    '#language' => $langcode,
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'harmony_core') . '/css/harmony-core.css',
      ),
    ),
  );

  // Extra fields.
  $extra_fields = field_info_extra_fields('harmony_post', 'harmony_post', 'display');

  if (isset($extra_fields['harmony_core_post_revision_display_log']['display'][$view_mode]['visible']) && $extra_fields['harmony_core_post_revision_display_log']['display'][$view_mode]['visible']) {
    $revision_log = harmony_core_post_revision_display_log($post, $view_mode);

    if (!empty($revision_log)) {
      $build['harmony_core_post_revision_display_log'] = array(
        '#markup' => $revision_log,
      );
    }
  }

  // Allow modules to modify the structured post.
  $type = 'harmony_post';
  drupal_alter(array('harmony_post_view', 'entity_view'), $build, $type);

  return $build;
}

function harmony_core_post_build_content($post, $view_mode = 'full', $langcode = NULL) {
  global $user;
  if (!isset($langcode)) {
    $langcode = $GLOBALS['language_content']->language;
  }

  // Remove previously built content, if exists.
  $post->content = array();
  $wrapper = entity_metadata_wrapper('harmony_post', $post);
  $thread_id = $wrapper->field_harmony_thread->value() ? $wrapper->field_harmony_thread->thread_id->value() : NULL;
  // Load of this will be cached.
  $thread_properties = $thread_id ? harmony_core_thread_get_properties($thread_id) : array();

  // Allow modules to change the view mode.
  $context = array(
    'entity_type' => 'harmony_post',
    'entity' => $post,
    'langcode' => $langcode,
  );
  drupal_alter('entity_view_mode', $view_mode, $context);

  // Build fields content.
  // In case of a multiple view, harmony_core_post_view_multiple() already ran the
  // 'prepare_view' step. An internal flag prevents the operation from running
  // twice.
  field_attach_prepare_view('harmony_post', array($post->post_id => $post), $view_mode, $langcode);
  entity_prepare_view('harmony_post', array($post->post_id => $post), $langcode);
  $post->content += field_attach_view('harmony_post', $post, $view_mode, $langcode);

  // Posts have links, we need to collect them together.
  $links = array();
  $post->content['links'] = array(
    '#theme' => 'links__post',
    '#pre_render' => array('drupal_pre_render_links'),
    '#attributes' => array('class' => array('post-links', 'inline')),
  );
  // Check for direct replies.
  if ($view_mode != 'inline_reply' && user_access('access show replies')) {
    $post_replies = harmony_post_get_replies($post->post_id);
    if (!empty($post_replies)) {
      $links['show_replies'] = array(
        'title' => format_plural(count($post_replies), '1 reply', '@count replies'),
        'href' => '',
        'attributes' => array(
          'id' => "post-$post->post_id-show-replies",
          'class' => array(
            'post-show-replies',
          ),
          'data-post-id' => $post->post_id,
        ),
      );
      drupal_add_library('system', 'drupal.ajax');
      drupal_add_js(drupal_get_path('module', 'harmony_core') . '/js/harmony-post.js');
    }
  }
  // Edit link.
  if (!empty($thread_properties) && !$thread_properties['is_locked'] && harmony_core_post_access_callback('update', $post)) {
    $links['edit'] = array(
      'title' => t('Edit'),
      'href' => "post/$post->post_id/edit",
      'attributes' => array('title' => t('Edit this post')),
    );
    $edit_query = array();

    if ($thread_id) {
      // If this is the first post in a thread, edit the thread and not the post.
      if ($post->first_post) {
        $links['edit']['href'] = "thread/$thread_id/edit";
      }

      $edit_query = array(
        'destination' => url("thread/$thread_id"),
      );

      if (!empty($_GET['page']) && is_numeric($_GET['page'])) {
        $edit_query['destination'] = url("thread/$thread_id", array('query' => array('page' => $_GET['page'])));
      }
      else {
        $edit_query['destination'] = url("thread/$thread_id");
      }
    }

    $links['edit']['query'] = $edit_query;
  }
  // Reply links.
  // Only show if this thread is open.
  $show_reply_to_anon = !empty($thread_properties) ? variable_get('harmony_core_bundle__' . $thread_properties['type'] . '__show_anon_reply_links', 0) : 0;
  if (!empty($thread_properties) && $thread_properties['can_post_in'] && (user_access('create harmony_posts') || $show_reply_to_anon)) {
    $links['reply'] = array(
      'title' => t('Reply'),
      'href' => 'post/add',
      'attributes' => array('title' => t('Reply directly to this post')),
      'query' => array(
        'field_harmony_thread' => $thread_id,
        'field_harmony_post_is_reply_to' => $post->post_id,
      ),
    );

    // For anonymous ask them to login first!
    if (!$user->uid) {
      $reply_url = url($links['reply']['href'], $links['reply']);

      $links['reply']['href'] = 'user';
      $links['reply']['query'] = array(
        'destination' => $reply_url,
      );
    }
  }
  if (user_access('reply as new harmony_thread') || $show_reply_to_anon) {
    $links['reply_as_new_thread'] = array(
      'title' => t('Reply as a new thread'),
      'href' => 'thread/add',
      'attributes' => array('title' => t('Create a new thread as a reply to this post')),
      'query' => array(
        'field_harmony_thread_cont_from' => $post->post_id,
      ),
    );

    // For anonymous ask them to login first!
    if (!$user->uid) {
      $reply_as_new_url = url($links['reply_as_new_thread']['href'], $links['reply_as_new_thread']);

      $links['reply_as_new_thread']['href'] = 'user';
      $links['reply_as_new_thread']['query'] = array(
        'destination' => $reply_as_new_url,
      );
    }
  }
  $post->content['links']['post'] = array(
    '#theme' => 'links__post__post',
    '#links' => $links,
    '#attributes' => array('class' => array('inline')),
  );

  // Allow modules to make their own additions to the post.
  module_invoke_all('harmony_post_view', $post, $view_mode, $langcode);
  module_invoke_all('entity_view', $post, 'harmony_post', $view_mode, $langcode);

  // Make sure the current view mode is stored if no module has already
  // populated the related key.
  $post->content += array('#view_mode' => $view_mode);
}

/**
 * Form submission handler for harmony_core_post_form().
 *
 * Handles the 'Delete' button on the post form.
 *
 * @see harmony_core_post_form()
 */
function harmony_post_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $post = $form['post']['#value'];
  $form_state['redirect'] = array('post/' . $post->post_id . '/delete', array('query' => $destination));
}

function harmony_post_delete_confirm($form, &$form_state, $post) {
  $form['#post'] = $post;

  // Always provide entity id in the same form key as in the entity edit form.
  $form['post_id'] = array('#type' => 'value', '#value' => $post->post_id);

  // Check that this isn't the first post in a thread. If it is, prevent deletion.
  if ($post->first_post) {
    $wrapper = entity_metadata_wrapper('harmony_post', $post);
    $form['text']['#markup'] = '<p>' . t("You cannot delete this post as it's the first in a thread. You must delete the thread.") . '</p>';

    if ($wrapper->field_harmony_thread->value()) {
      $thread_id = $wrapper->field_harmony_thread->thread_id->value();
      $form['text']['#markup'] .= '<p>' . l(t('Return to thread'), "thread/$thread_id") . '</p>';
    }
  }
  else {
    $form = confirm_form($form,
      t('Are you sure you want to delete %title?', array('%title' => $post->title)),
      'post/' . $post->post_id,
      t('This action cannot be undone.'),
      t('Delete'),
      t('Cancel')
    );
  }

  return $form;
}

/**
 * Executes post deletion.
 *
 * @see harmony_post_delete_confirm()
 */
function harmony_post_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $post = harmony_post_load($form_state['values']['post_id']);
    harmony_post_delete($post->post_id);

    watchdog('harmony', 'Post: deleted %title.', array('%title' => $post->title));
    drupal_set_message(t('The post %title has been deleted.', array('%title' => $post->title)));
  }

  $form_state['redirect'] = '<front>';
}

/**
 * Page title callback for post revision page.
 */
function harmony_core_post_revision_title($post) {
  $author = user_load($post->uid);
  return t('Revision from @date by @author', array('@date' => format_date($post->timestamp), '@author' => $author->name));
}

function harmony_core_post_revisions_view($post, $view_mode = 'full') {
  return views_embed_view(variable_get('harmony_core_post_revisions_view', 'harmony_post_revisions'), 'default', $post->post_id);
}

/**
 * Page or ctools AJAX  callback, returns post diff output.
 */
function harmony_core_post_revision_view($new_post, $vid, $js = NULL, $view_mode = 'full') {
  // Figure out the previous revision.
  $previous_vid = db_query_range('SELECT vid FROM {harmony_post_revision} WHERE post_id = :post_id AND vid < :vid ORDER BY vid DESC', 0, 1, array(':post_id' => $new_post->post_id, ':vid' => $vid))->fetchField();

  if ($previous_vid) {
    $diff_theme = variable_get('diff_theme', 'default');
    drupal_add_css(drupal_get_path('module', 'diff') . "/css/diff.{$diff_theme}.css");

    // Generate the diff.
    $old_post = harmony_post_load($new_post->post_id, $previous_vid);
    $old_post_wrapper = entity_metadata_wrapper('harmony_post', $old_post);
    $new_post_wrapper = entity_metadata_wrapper('harmony_post', $new_post);

    // drupal alter this stuff, let people change what we diff.
    $stuff_to_diff = array(
      'title' => array(
        'title' => t('Post title'),
        'theme_function' => '',
        'property' => 'title',
      ),
      'post_text' => array(
        'title' => t('Post text'),
        'theme_function' => '',
        'property' => 'field_harmony_text',
      ),
    );

    drupal_alter('harmony_post_diff_properties', $stuff_to_diff);

    // Add the diff header.
    $output = theme('harmony_post_diff_inline_metadata', array('post' => $new_post));

    foreach ($stuff_to_diff as $thing) {
      $old_value = $old_post_wrapper->{$thing['property']}->value(array('sanitize' => TRUE));
      if (!empty($old_value['safe_value'])) {
        $old_value = $old_value['safe_value'];
      }
      else {
        $old_value = '';
      }

      $new_value = $new_post_wrapper->{$thing['property']}->value(array('sanitize' => TRUE));
      if (!empty($new_value['safe_value'])) {
        $new_value = $new_value['safe_value'];
      }
      else {
        $new_value = '';
      }

      /* @todo implement and make proper use of theme functions here.  */
      $output .= diff_get_inline($old_value, $new_value);
    }
  }
  else {
    $output = t('No previous revisions were found.');
  }

  // Return in the method you are accustomed to.
  if ($js) {
    ctools_include('ajax');
    ctools_include('modal');

    $title = harmony_core_post_revision_title($new_post);
    print ctools_modal_render($title, $output);

    global $devel_shutdown;
    $devel_shutdown = FALSE;

    drupal_exit();
  }
  else {
    return $output;
  }
}

/**
 * harmony_core_ajax_post_replies().
 *
 * Return rendered HTML list of replies to a particular post.
 */
function harmony_core_ajax_post_replies($post, $view_mode = 'full', $langcode = NULL) {
  $post_replies = views_embed_view(variable_get('harmony_core_post_replies_view', 'harmony_post_replies_to_post'), 'default', $post->post_id);

  return array(
    '#type' => 'ajax',
    '#commands' => array(
      array(
        'command' => 'harmony_post_show_replies',
        'selector' => "#post-{$post->post_id}-replies",
        'data' => $post_replies,
      ),
    ),
  );
}

/**
 * Asks for confirmation of the reversion to prevent against CSRF attacks.
 *
 * @param int $post_revision
 *   The post revision ID.
 *
 * @return array
 *   An array as expected by drupal_render().
 *
 * @see harmony_core_menu()
 * @see post_revision_revert_confirm_submit()
 * @ingroup forms
 */
function harmony_core_post_revision_revert_confirm($form, $form_state, $post_revision) {
  $form['#post_revision'] = $post_revision;
  return confirm_form($form, t('Are you sure you want to revert to the revision from %revision-date?', array('%revision-date' => format_date($post_revision->timestamp))), 'post/' . $post_revision->post_id . '/revisions', '', t('Revert'), t('Cancel'));
}

/**
 * Form submission handler for harmony_core_post_revision_revert_confirm().
 */
function harmony_core_post_revision_revert_confirm_submit($form, &$form_state) {
  $post_revision = $form['#post_revision'];
  $post_revision->revision = 1;
  $post_revision->log = t('Copy of the revision from %date.', array('%date' => format_date($post_revision->timestamp)));

  harmony_post_save($post_revision);

  watchdog('harmony', 'Post: reverted %title revision %revision.', array('%title' => $post_revision->title, '%revision' => $post_revision->vid));
  drupal_set_message(t('Post %title has been reverted back to the revision from %revision-date.', array('%title' => $post_revision->title, '%revision-date' => format_date($post_revision->timestamp))));
  $form_state['redirect'] = 'post/' . $post_revision->post_id . '/revisions';
}

/**
 * Form constructor for the revision deletion confirmation form.
 *
 * This form prevents against CSRF attacks.
 *
 * @param $post_revision
 *   The post revision ID.
 *
 * @return
 *   An array as expected by drupal_render().
 *
 * @see harmony_core_menu()
 * @see harmony_core_post_revision_delete_confirm_submit()
 * @ingroup forms
 */
function harmony_core_post_revision_delete_confirm($form, $form_state, $post_revision) {
  $form['#post_revision'] = $post_revision;
  return confirm_form($form, t('Are you sure you want to delete the revision from %revision-date?', array('%revision-date' => format_date($post_revision->timestamp))), 'post/' . $post_revision->post_id . '/revisions', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}

/**
 * Form submission handler for harmony_core_post_revision_delete_confirm().
 */
function harmony_core_post_revision_delete_confirm_submit($form, &$form_state) {
  $post_revision = $form['#post_revision'];
  $result = harmony_post_revision_delete($post_revision->vid);

  if ($result) {
    watchdog('harmony', 'Deleted %title revision %revision.', array('%title' => $post_revision->title, '%revision' => $post_revision->vid));
    drupal_set_message(t('Revision from %revision-date %title has been deleted.', array('%revision-date' => format_date($post_revision->timestamp), '%title' => $post_revision->title)));
    $form_state['redirect'] = 'post/' . $post_revision->post_id;
    if (db_query('SELECT COUNT(vid) FROM {post_revision} WHERE post_id = :post_id', array(':post_id' => $post_revision->post_id))->fetchField() > 1) {
      $form_state['redirect'] .= '/revisions';
    }
  }
  else {
    drupal_set_message(t('Failed to delete revision.'), 'error');
  }
}
