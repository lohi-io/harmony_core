<?php

/**
 * @file
 * harmony_core.actions.inc
 */

/**
 * Sets the status of a thread to 1 (published).
 *
 * @param $thread
 *   A thread object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_thread_publish_action($thread, $context = array()) {
  $thread->status = HARMONY_PUBLISHED;
  watchdog('action', 'Set thread %title to published.', array('%title' => $thread->title));
  /* @todo queue unpublish of posts. */
}

/**
 * Sets the status of a thread to 0 (unpublished).
 *
 * @param $thread
 *   A thread object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_thread_unpublish_action($thread, $context = array()) {
  $thread->status = HARMONY_NOT_PUBLISHED;
  watchdog('action', 'Set thread %title to unpublished.', array('%title' => $thread->title));
}

/**
 * Sets the pinned-at-top-of-list property of a thread to 1.
 *
 * @param $thread
 *   A thread object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_thread_make_pinned_action($thread, $context = array()) {
  $thread->pinned = HARMONY_PINNED;
  watchdog('action', 'Set thread %title to pinned.', array('%title' => $thread->title));
}

/**
 * Sets the pinned-at-top-of-list property of a thread to 0.
 *
 * @param $thread
 *   A thread object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_thread_make_unpinned_action($thread, $context = array()) {
  $thread->pinned = HARMONY_NOT_PINNED;
  watchdog('action', 'Set thread %title to unpinned.', array('%title' => $thread->title));
}

/**
 * Sets the locked property of a thread to 1.
 *
 * @param $thread
 *   A thread object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_thread_locked_action($thread, $context = array()) {
  $thread->locked = HARMONY_LOCKED;
  watchdog('action', 'Locked the thread %title.', array('%title' => $thread->title));
}

/**
 * Sets the lock property of a thread to 0.
 *
 * @param $thread
 *   A thread object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_thread_unlock_action($thread, $context = array()) {
  $thread->locked = HARMONY_NOT_LOCKED;
  watchdog('action', 'Unlocked the thread %title.', array('%title' => $thread->title));
}

/**
 * Sets the hidden property of a thread to 1 (hidden).
 *
 * @param $thread
 *   A thread object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_thread_hide_action($thread, $context = array()) {
  $thread->hidden = HARMONY_HIDDEN;
  watchdog('action', 'Hidden the thread %title.', array('%title' => $thread->title));
}

/**
 * Sets the hidden property of a thread to 0 (unhidden).
 *
 * @param $thread
 *   A thread object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_thread_unhide_action($thread, $context = array()) {
  $thread->hidden = HARMONY_NOT_HIDDEN;
  watchdog('action', 'Unhidden the thread %title.', array('%title' => $thread->title));
}

/**
 * Saves a thread.
 *
 * @param $thread
 *   The thread to be saved.
 *
 * @ingroup actions
 */
function harmony_core_thread_save_action($thread) {
  harmony_thread_save($thread);
  watchdog('action', 'Saved thread %title', array('%title' => $thread->title));
}

/**
 * Move thread to a category form callback.
 */
function harmony_core_thread_move_category_action_form($settings) {
  global $user;
  $form = array();
  $form_state = array();

  $thread = harmony_core_thread_new_object();

  $category_field = variable_get('harmony_core_category_field', 'field_harmony_category');
  field_attach_form('harmony_thread', $thread, $form, $form_state, NULL, array('field_name' => $category_field));

  $form[$category_field][$form[$category_field]['#language']]['#title'] = t('Category to move threads to');
  $form[$category_field][$form[$category_field]['#language']]['#required'] = TRUE;

  // Allow other modules to mess with the options for access control reasons.
  drupal_alter('harmony_category_options', $form[$category_field][$form[$category_field]['#language']]['#options']);
  $options = $form[$category_field][$form[$category_field]['#language']]['#options'];
  // If empty, hide and throw out a helpful message.
  if (empty($options) || !empty($options) && count($options) === 1 && isset($options['_none'])) {
    $form[$category_field]['#access'] = FALSE;
    $form['guidance_text'] = t("<p>You don't have access to any categories or there aren't any categories.</p>");
  }

  return $form;
}

/**
 * Action form, form submit.
 */
function harmony_core_thread_move_category_action_submit($form, &$form_state) {
  $return = array();
  $category_field = variable_get('harmony_core_category_field', 'field_harmony_category');
  $lang = !empty($form['#entity']->language) ? $form['#entity']->language : LANGUAGE_NONE;

  $return['term_id'] = $form_state['values'][$category_field][$lang][0]['tid'];

  return $return;
}

/**
 * Action for moving a thread to a category.
 */
function harmony_core_thread_move_category_action(&$thread, $context) {
  $wrapper = entity_metadata_wrapper('harmony_thread', $thread);

  // Check that the thread has the category field.
  $category_field = variable_get('harmony_core_category_field', 'field_harmony_category');
  if ($wrapper->__isset($category_field)) {
    $wrapper->{$category_field}->set($context['term_id']);
  }
}

/**
 * Publish a post, this function is smart enough to know when it's the only
 * post in a thread and to re-publish that thread if it's not published and
 *
 * @param $post
 *   A post object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_post_publish_action($post, $context = array()) {
  $wrapper = entity_metadata_wrapper('harmony_post', $post);
  $thread_id = $wrapper->field_harmony_thread->thread_id->value();

  if ($thread_id && $post->hidden == HARMONY_NOT_HIDDEN && $wrapper->field_harmony_thread->status == HARMONY_NOT_PUBLISHED) {
    $thread = harmony_thread_load($thread_id);
    harmony_core_thread_publish_action($thread);
    harmony_thread_save($thread);
    watchdog('action', 'Whilst republishing a post the thread %id was also republished..', array('%id' => $thread_id));
  }

  $post->status = HARMONY_PUBLISHED;
  watchdog('action', 'Set post %title to published.', array('%title' => $post->title));
}

/**
 * Unpublish a post, this action will behave particular ways depending on if...
 * - it's the only post in a thread, in which case unpublish the thread
 * - it's not the only post in a thread so just unpublish the post
 *
 * @param $post
 *   A post object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_post_unpublish_action($post, $context = array()) {
  $wrapper = entity_metadata_wrapper('harmony_post', $post);
  $thread_id = $wrapper->field_harmony_thread->thread_id->value();

  $thread_posts = harmony_core_get_thread_posts($thread_id);

  if (in_array($post->post_id, $thread_posts)) {
    // If it's the only post in a thread then unpublish the thread instead.
    if (count($thread_posts) === 1) {
      $thread = harmony_thread_load($thread_id);
      harmony_core_thread_unpublish_action($thread);
      harmony_thread_save($thread);

      watchdog('action', "Somone tried to unpublish post %post_id, as it's the only post in the thread the thread was unpublished instead. The thread that was unpublished was %thread_id.", array('%post_id' => $post->post_id, '%thread_id' => $thread_id));
    }
    else {
      $post->status = HARMONY_NOT_PUBLISHED;
      watchdog('action', 'Post %id was unpublished.', array('%id' => $post->post_id));
    }
  }
}

/**
 * Sets the locked property of a post to 1.
 *
 * @param $post
 *   A post object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_post_locked_action($post, $context = array()) {
  $post->locked = HARMONY_LOCKED;
  watchdog('action', 'Locked a post in the thread %title.', array('%title' => $post->title));
}

/**
 * Sets the lock property of a post to 0.
 *
 * @param $post
 *   A post object.
 * @param $context
 *   (optional) Array of additional information about what triggered the action.
 *   Not used for this action.
 *
 * @ingroup actions
 */
function harmony_core_post_unlock_action($post, $context = array()) {
  $post->locked = HARMONY_NOT_LOCKED;
  watchdog('action', 'Unlocked a post in the thread %title.', array('%title' => $post->title));
}

function harmony_core_action_get_thread_entityreference_form(&$form, &$form_state) {
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

  field_attach_form('harmony_post', $post, $form, $form_state, NULL, array('field_name' => 'field_harmony_thread'));

  // Entity reference prepopulate will be hiding this field.
  $form['field_harmony_thread']['#access'] = TRUE;

  // No meta tags here plz, gets everywhere.
  unset($form['#metatags']);
}

function harmony_core_action_get_thread_entityreference_form_submit($form, $form_state) {
  $return = array();
  $lang = !empty($form['#entity']->language) ? $form['#entity']->language : LANGUAGE_NONE;

  $thread_id = $form_state['values']['field_harmony_thread'][$lang][0]['target_id'];
  // Load the thread here once so we can pass along the title.
  $thread = harmony_thread_load($thread_id);

  $return['thread'] = $thread;
  $return['thread_id'] = $thread_id;
  $return['thread_title'] = $thread->title;

  return $return;
}

function harmony_core_thread_merge_action_form($settings) {
  $form = array();
  $form_state = array();

  // Help text.
  $form['guidance_text'] = array(
    '#markup' => t('<p>Posts from the selected threads will be moved to the specified one, and the selected threads deleted.'),
  );

  harmony_core_action_get_thread_entityreference_form($form, $form_state);

  // Change the title.
  $form['field_harmony_thread'][$form['#entity']->language][0]['target_id']['#title'] = t('Thread to merge post(s) in to');

  return $form;
}

function harmony_core_thread_merge_action_submit($form, &$form_state) {
  return harmony_core_action_get_thread_entityreference_form_submit($form, $form_state);
}

function harmony_core_thread_merge_action(&$thread, $context) {
  // Check we're not trying to merge a thread in to ...itself.
  if ($thread->thread_id === $context['thread_id']) {
    drupal_set_message(t('You selected a thread to merge with itself, skipping.'), 'error');
  }
  else {
    $thread_wrapper = entity_metadata_wrapper('harmony_thread', $thread);

    // Get a list of posts in the current thread.
    $query = new EntityFieldQuery;
    $query_result = $query
      ->entityCondition('entity_type', 'harmony_post')
      ->fieldCondition('field_harmony_thread', 'target_id', $thread->thread_id)
      ->execute();

    if (!empty($query_result['harmony_post'])) {
      // Get the first post in the target/new thread.
      // We need to re-assess each time what the first post is.
      $first_post_query = new EntityFieldQuery;
      $first_post_result = $first_post_query
        ->entityCondition('entity_type', 'harmony_post')
        ->fieldCondition('field_harmony_thread', 'target_id', $context['thread_id'])
        ->propertyOrderBy('created')
        ->range(0, 1)
        ->execute();

      $first_post_updated = FALSE;
      $first_post_timestamp = 0;
      if (!empty($first_post_result['harmony_post'])) {
        $first_post_id = array_shift(array_keys($first_post_result['harmony_post']));
        $first_post = harmony_post_load($first_post_id);
        $first_post_wrapper = entity_metadata_wrapper('harmony_post', $first_post);
        $first_post_timestamp = $first_post_wrapper->created->value();
      }

      // Flatten the post id array a bit and load the posts.
      $post_ids = array_keys($query_result['harmony_post']);
      $posts = harmony_post_load_multiple($post_ids);
      $posts_count = count($posts);
      $post_ids_created = array();

      if (!empty($posts)) {
        foreach ($posts as $post) {
          $wrapper = entity_metadata_wrapper('harmony_post', $post);

          // Set the post thread and new title.
          $wrapper->title->set($context['thread_title']);
          $wrapper->field_harmony_thread->set($context['thread_id']);

          // If the post was created before the current first post, re-assign.
          if ($wrapper->created->value() < $first_post_timestamp) {
            $wrapper->first_post->set(1);
            $first_post_updated = TRUE;
          }
          else {
            $wrapper->first_post->set(0);
          }

          // Unset last post if found as we'll just set this later on.
          $wrapper->last_post->set(0);

          // Save the post ids to an array which we'll sort to determine
          // which post is the last.
          $post_ids_created[$post->post_id] = $wrapper->created->value();

          // Save changes.
          $wrapper->save();
        }
      }

      // If first post isn't first anymore, update.
      if ($first_post_updated) {
        $first_post_wrapper->first_post->set(0);
        $first_post_wrapper->save();
      }

      // Work out the last post and set.
      asort($post_ids_created);
      $last_post_id = array_pop($post_ids_created);
      $last_post = harmony_post_load($last_post_id);
      $last_post_wrapper = entity_metadata_wrapper('harmony_post', $last_post);
      $last_post_wrapper->last_post->set(1);
      $last_post_wrapper->save();

      // Set the post count & likes to 0, and delete.
      $likes_count = $thread_wrapper->likes->value();
      $thread_wrapper->field_harmony_post_count->set(0);
      $thread_wrapper->likes->set(0);
      $thread_wrapper->save();
      harmony_thread_delete($thread->thread_id);

      // Update the target thread's post count & likes.
      $target_thread_wrapper = entity_metadata_wrapper('harmony_thread', $context['thread']);
      $new_post_count = $target_thread_wrapper->field_harmony_post_count->value() + $posts_count;
      $target_thread_wrapper->field_harmony_post_count->set($new_post_count);
      $new_likes_count = $target_thread_wrapper->likes->value() + $likes_count;
      $target_thread_wrapper->likes->set($new_likes_count);
      $target_thread_wrapper->field_harmony_last_post->set($last_post_id);
      $target_thread_wrapper->save();

      $message_vars = array(
        '@old_thread_title' => $thread->title,
        '@target_thread' => $context['thread_title'],
      );
      drupal_set_message(t('Posts from the thread &quot;@old_thread_title&quot; to &quot;@target_thread&quot;.', $message_vars));
    }
  }
}

function harmony_core_post_reassign_action_form($settings) {
  $form = array();
  $form_state = array();

  harmony_core_action_get_thread_entityreference_form($form, $form_state);

  // Change the title.
  $form['field_harmony_thread'][$form['#entity']->language][0]['target_id']['#title'] = t('Thread to re-assign post(s) to');

  return $form;
}

function harmony_core_post_reassign_action_validate($form, &$form_state) {
  $lang = !empty($form['#entity']->language) ? $form['#entity']->language : LANGUAGE_NONE;
  $view_arg = !empty($form_state['build_info']['args'][0]->args[0]) && ctype_digit((string) $form_state['build_info']['args'][0]->args[0]) ? $form_state['build_info']['args'][0]->args[0] : 0;

  // Check we're not moving posts to the same thread as they're in now.
  if ($form_state['values']['field_harmony_thread'][$lang][0]['target_id'] === $view_arg) {
    form_set_error('field_harmony_thread', t("You cannot move posts to the same thread as they're in now."));
  }
}

function harmony_core_post_reassign_action_submit($form, &$form_state) {
  return harmony_core_action_get_thread_entityreference_form_submit($form, $form_state);
}

function harmony_core_post_reassign_action(&$post, $context) {
  // We can't re-assign the first post otherwise there wouldn't be
  // anything in the thread, instead recommend merging threads.
  // @todo code to allow re-assigning of first post.
  if ($post->first_post) {
    $error_vars = array(
      '@post_id' => $post->post_id,
      '@thread_merge_url' => url('admin/harmony/content/threads'),
    );
    drupal_set_message(t('You cannot move the first post in a thread (Post ID: @post_id), if you wish to do this you can <a href="@thread_merge_url">merge the thread with another</a>.', $error_vars), 'error');
  }
  else {
    $wrapper = entity_metadata_wrapper('harmony_post', $post);

    // Hold on to the old thread_id. @todo, verify this works :|
    $old_thread_id = $wrapper->field_harmony_thread->thread_id->value();

    // Get the number of likes against this post, and update the old thread.
    $post_likes = NULL;
    $likes = flag_get_counts('harmony_post', $post->post_id);
    if (!empty($likes['harmony_likes'])) {
      $post_likes = $likes['harmony_likes'];
      $old_thread_likes = $wrapper->field_harmony_thread->likes->value();
      $wrapper->field_harmony_thread->likes->set($old_thread_likes - $post_likes);
      $wrapper->field_harmony_thread->save();
    }

    // Setup the new values, save a little later.
    $wrapper->title->set($context['thread_title']);
    $wrapper->field_harmony_thread->set($context['thread_id']);

    $message_vars = array(
      '@post_id' => $post->post_id,
      '%thread_title' => $context['thread_title'],
    );

    // Is this post the last in the old thread? If so update the old thread.
    if ($wrapper->last_post->value()) {
      $old_thread = harmony_thread_load($old_thread_id);
      $old_thread_posts = harmony_core_get_thread_posts($old_thread->thread_id);

      // Get last item in the array.
      $new_last_post_id = array_pop($thread_posts);

      // Update the thread.
      $old_thread_wrapper = entity_metadata_wrapper('harmony_thread', $old_thread);
      $old_thread_wrapper->field_harmony_last_post->set($new_last_post_id);
      $old_thread_wrapper->save();

      // Update the post.
      if ($post->post_id == $new_last_post_id) {
        $wrapper->last_post->set(1);
      }
      else {
        // Update the last post property on the new last post.
        $new_last_post = harmony_post_load($new_last_post_id);
        $new_last_post_wrapper = entity_metadata_wrapper('harmony_post', $new_last_post);
        $new_last_post_wrapper->last_post->set(1);
        $new_last_post_wrapper->save();
      }
    }

    // Save the current post.
    $wrapper->save();

    // Update the new thread's likes.
    if ($post_likes) {
      $new_thread_likes = $wrapper->field_harmony_thread->likes->value();
      $wrapper->field_harmony_thread->likes->set($new_thread_likes + $post_likes);
      $wrapper->field_harmony_thread->save();
    }

    // Decrement the old thread's post count.
    harmony_core_update_meta_count(array('field_harmony_post_count'), 'harmony_thread', $old_thread_id, 'decrement');

    // Increment the new thread's.
    harmony_core_update_meta_count(array('field_harmony_post_count'), 'harmony_thread', $context['thread_id']);

    // Inform the user.
    drupal_set_message(t('Post ID: @post_id re-assigned to the thread &quot;%thread_title&quot;.', $message_vars));
  }
}

function harmony_core_post_split_action_form($settings, $vbo = array()) {
  global $user;
  $form = array();
  $form_state = array();

  // Build a standard thread object.
  $thread = harmony_core_thread_new_object();
  $thread->updated = 0;

  // Attempt to grab the first selected post which will be the base of
  // information for the new thread.
  if (!empty($vbo['selection']) && is_array($vbo['selection'])) {
    // Attempt to load the entity.
    $post_id = reset($vbo['selection']);
    $post = harmony_post_load($post_id);

    // @todo Check to see if this post is published, load the next if not.
    if ($post && $post->status == HARMONY_NOT_PUBLISHED) {
      foreach ($vbo['selection'] as $vbo_post_id) {
        $post = harmony_post_load($vbo_post_id);

        if ($post->status == HARMONY_PUBLISHED) {
          break;
        }
      }
    }

    if ($post) {
      $wrapper = entity_metadata_wrapper('harmony_post', $post);

      $thread->title = $wrapper->field_harmony_thread->title->value();
      $thread->type = $wrapper->field_harmony_thread->type->value();
    }
  }

  // Create form elements, straight copy from harmony_core_thread_form().
  // Previously tried using the return of this function but it resulted
  // in unusual behaviour and fieldapi values not being set.
  $form['thread'] = array(
    '#type' => 'value',
    '#value' => $thread,
  );
  foreach (array('thread_id', 'uid', 'is_new') as $key) {
    $form[$key] = array(
      '#type' => 'value',
      '#value' => isset($thread->$key) ? $thread->$key : NULL,
    );
  }

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Thread title'),
    '#required' => TRUE,
    '#default_value' => $thread->title,
    '#attributes' => array(
      'placeholder' => t('Thread title'),
    ),
    '#title_display' => 'invisible',
    '#maxlength' => 255,
  );

  // Attach fields.
  field_attach_form('harmony_thread', $thread, $form, $form_state);

  // Kill off a bunch of stuff we don't want.
  $form['field_harmony_first_post']['#access'] = FALSE;
  $form['field_harmony_last_post']['#access'] = FALSE;
  $form['field_harmony_last_user_to_reply']['#access'] = FALSE;
  $form['field_harmony_post_count']['#access'] = FALSE;

  // Options for administrators
  $form['options'] = array(
    '#type' => 'fieldset',
    '#access' => user_access('administer harmony forum content'),
    '#title' => t('Thread options'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#group' => 'additional_settings',
    '#attributes' => array(
      'class' => array('thread-form-options'),
    ),
    '#weight' => 95,
  );
  $form['options']['status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Published'),
    '#default_value' => $thread->status,
  );
  $form['options']['pinned'] = array(
    '#type' => 'checkbox',
    '#title' => t('Pin to the top of listings'),
    '#default_value' => $thread->pinned,
  );
  $form['options']['locked'] = array(
    '#type' => 'checkbox',
    '#title' => t('Lock from replies and edits'),
    '#default_value' => $thread->locked,
  );
  $form['options']['hidden'] = array(
    '#type' => 'checkbox',
    '#title' => t('Hide from listings'),
    '#default_value' => $thread->hidden,
  );
  $form['options']['override_created'] = array(
    '#type' => 'checkbox',
    '#title' => t('Set the new threads created date to now'),
    '#default_value' => 0,
    '#description' => t("Tick to change the new threads created date to be now. This would be instead of taking the oldest post (first in the new thread) selecteds created date."),
  );

  // Add in help text and an author field for re-assigning.
  $form['guidance_text'] = array(
    '#markup' => t('<p>To split posts in to another thread, we must create a new thread. Please complete the form below.'),
    '#weight' => -100,
  );

  $form['author'] = array(
    '#type' => 'textfield',
    '#title' => t('Thread author'),
    '#default_value' => $user->name,
    '#required' => TRUE,
    '#autocomplete_path' => 'user/autocomplete',
    '#maxlength' => '60',
  );

  // Prepopulate values.
  if (!empty($post) && $wrapper) {
    $form['author']['#default_value'] = $wrapper->uid->name->value();
    $form['author']['#description'] = t('The author has been set by default to the author of the first published selected post.');

    // Taxonomy if the bundle has that field.
    $category_field = variable_get('harmony_core_category_field', 'field_harmony_category');
    if (!empty($form[$category_field]) && $wrapper->field_harmony_thread->{$category_field}->value()) {
      $form[$category_field][$form[$category_field]['#language']]['#default_value'] = $wrapper->field_harmony_thread->{$category_field}->tid->value();
    }
  }

  return $form;
}

function harmony_core_post_split_action_validate($form, &$form_state) {
  field_attach_form_validate('harmony_thread', $form_state['values']['thread'], $form, $form_state);
}

function harmony_core_post_split_action_submit($form, &$form_state) {
  $thread = $form_state['values']['thread'];
  $is_new = $thread->is_new;
  $wrapper = entity_metadata_wrapper('harmony_thread', $thread);

  // Setup date overrides if in play.
  if ($form_state['values']['override_created']) {
    $thread->override_created = TRUE;
    $thread->created = REQUEST_TIME;
    $thread->updated = REQUEST_TIME;
  }

  // Set the title from the form value.
  $wrapper->title->set($form_state['values']['title']);

  // Set the user, grab by name so we can get the uid.
  $author_user = user_load_by_name($form_state['values']['author']);

  if (!$author_user) {
    global $user;
    $author_user = $user;
  }
  $wrapper->uid->set($author_user->uid);

  // Set the thread meta properties.
  $wrapper->status->set($form_state['values']['status']);
  $wrapper->pinned->set($form_state['values']['pinned']);
  $wrapper->locked->set($form_state['values']['locked']);
  $wrapper->hidden->set($form_state['values']['hidden']);

  // Do submit for fieldapi fields.
  field_attach_submit('harmony_thread', $thread, $form, $form_state);

  // Set the post count to 0 as the subsequent posts will update the count.
  // Field API will enforce it's default value (1) which we don't want.
  $wrapper->field_harmony_post_count->set(0);

  // Do a full save of the thread.
  $thread = harmony_thread_save($thread);

  // Pass on the thread details for use in the action.
  return array(
    'thread' => $thread,
    'thread_id' => $thread->thread_id,
    'thread_title' => $thread->title,
  );
}

function harmony_core_post_split_action(&$post, $context) {
  // We can't re-assign the first post otherwise there wouldn't be
  // anything in the thread, instead recommend merging threads.
  // @todo code to allow re-assigning of first post.
  if ($post->first_post) {
    $error_vars = array(
      '@post_id' => $post->post_id,
      '@thread_merge_url' => url('admin/harmony/content/threads'),
    );
    drupal_set_message(t('You cannot move the first post in a thread (Post ID: @post_id), if you wish to do this you can <a href="@thread_merge_url">merge the thread with another</a>.', $error_vars), 'error');
  }
  else {
    // Push title to posts, set the updated on the thread.
    $thread = $context['thread'];
    $thread_id = $context['thread_id'];
    $thread_title = $context['thread_title'];

    // Wrappers!
    $wrapper = entity_metadata_wrapper('harmony_post', $post);
    $thread_wrapper = entity_metadata_wrapper('harmony_thread', $thread);

    // Grab the old thread.
    $old_thread_id = $wrapper->field_harmony_thread->thread_id->value();
    $old_thread = harmony_thread_load($old_thread_id);
    $old_thread_wrapper = entity_metadata_wrapper('harmony_thread', $old_thread);

    // Update post title.
    $post->title = $thread_title;

    // Update meta information.
    if ($post->status == HARMONY_PUBLISHED && $post->hidden == HARMONY_NOT_HIDDEN) {
      $update_first_post = FALSE;
      $update_last_post = FALSE;

      // Update timestamps if the post is published.
      if (!isset($thread->override_created)) {
        if ($post->created < $thread->created) {
          $thread->created = $post->created;
          $update_first_post = TRUE;
        }
        if ($post->updated > $thread->updated) {
          $thread->updated = $post->updated;
          $update_last_post = TRUE;
        }
      }

      // Update thread & category post counts if published. Isn't this done in
      // the controller? Nope as we bypass this by using harmony_thread_meta_save.
      // We need to increase and decrease values between new and old.
      // Don't use harmony_core_update_meta_count as we can do it faster here.
      $thread_post_count = $thread_wrapper->field_harmony_post_count->value();
      $thread_wrapper->field_harmony_post_count->set($thread_post_count + 1);

      // Decrease the old threads' post count.
      if ($old_thread->status == HARMONY_PUBLISHED) {
        $old_thread_post_count = $old_thread_wrapper->field_harmony_post_count->value();
        $old_thread_wrapper->field_harmony_post_count->set($old_thread_post_count - 1);
      }

      // Category now.
      $category_field = variable_get('harmony_core_category_field', 'field_harmony_category');
      if ($thread_wrapper->__isset($category_field) && $thread_wrapper->{$category_field}->value()) {
        $new_category_tid = $thread_wrapper->{$category_field}->tid->value();

        // Check to see if the post has in effect changed category, if so then
        // we need to decrement the value there as well.

        // If the old thread is published, has the category field and a value
        // then decrement. Don't increment the new category if the it's the
        // same as the old one and the new thread is published.
        $increment_new_category_count = $thread->status == HARMONY_PUBLISHED;
        if ($old_thread->status == HARMONY_PUBLISHED && $old_thread_wrapper->__isset($category_field) && $old_thread_wrapper->{$category_field}->value()) {
          if ($old_thread_wrapper->{$category_field}->tid->value() != $new_category_tid) {
            $old_category_post_count = $old_thread_wrapper->{$category_field}->field_harmony_post_count->value();
            $old_thread_wrapper->{$category_field}->field_harmony_post_count->set($old_category_post_count - 1);
            $old_thread_wrapper->{$category_field}->save();
          }
          // Category is the same, don't increment the new count.
          elseif ($thread->status == HARMONY_NOT_PUBLISHED) {
            $increment_new_category_count = FALSE;
          }
        }

        // Update the new category count.
        if ($increment_new_category_count) {
          $category_post_count = $thread_wrapper->{$category_field}->field_harmony_post_count->value();
          $thread_wrapper->{$category_field}->field_harmony_post_count->set($category_post_count + 1);
          $thread_wrapper->{$category_field}->save();
        }
      }

      // Likes too.
      // Get the number of likes against this post, and update the old thread.
      $post_likes = NULL;
      $likes = flag_get_counts('harmony_post', $post->post_id);
      if (!empty($likes['harmony_likes'])) {
        $post_likes = $likes['harmony_likes'];

        // Remove from the old thread.
        $old_thread_likes = $old_thread_wrapper->likes->value();
        $old_thread_wrapper->likes->set($old_thread_likes - $post_likes);

        // Add to the new one!
        $thread_likes = $thread_wrapper->likes->value();
        $thread_wrapper->likes->set($thread_likes + $post_likes);
      }

      // Should the move post action do this stuff too?
    }

    // Change the thread entity ref.
    $wrapper->field_harmony_thread->set($thread_id);

    // Save the post early.
    $post = harmony_post_meta_save($post);

    // Update the threads.
    $old_thread = harmony_thread_meta_save($old_thread);
    $thread = harmony_thread_meta_save($thread);

    // Rebuild the first and last thread properties.
    harmony_thread_rebuild_first_last_posts($old_thread);
    $thread_posts = harmony_thread_rebuild_first_last_posts($thread);

    // At this point as views bulk operations will save the entity for us it'll
    // override any changes made elsewhere unless we grab it here and re-apply.
    $post->first_post = $thread_posts['first'] == $post->post_id ? 1 : 0;
    $post->last_post = $thread_posts['last'] == $post->post_id ? 1 : 0;
  }
}
