<?php

/**
 * @file
 * Defines the inline entity form controller for Nodes.
 */

class HarmonyPostInlineEntityFormController extends EntityInlineEntityFormController {

  /**
  * Overrides EntityInlineEntityFormController::labels().
   */
  public function labels() {
    $labels = array(
      'singular' => t('post'),
      'plural' => t('posts'),
    );
    return $labels;
  }

  /**
   * Overrides EntityInlineEntityFormController::tableFields().
   */
  /*public function tableFields($bundles) {
    $fields = parent::tableFields($bundles);
    $fields['status'] = array(
      'type' => 'property',
      'label' => t('Status'),
      'weight' => 100,
    );

    return $fields;
  }*/

  /**
   * Overrides EntityInlineEntityFormController::entityForm().
   */
  public function entityForm($form, &$form_state) {
    global $user;

    $post_defaults = array(
      'title' => '',
      'uid' => $user->uid,
      'language' => LANGUAGE_NONE,
      'created' => REQUEST_TIME,
      'updated' => REQUEST_TIME,
    );

    $post = (object) array_merge($post_defaults, (array) $form['#entity']);

    $post->is_new = empty($post->post_id);

    $form['post'] = array(
      '#type' => 'value',
      '#value' => $post,
    );

    if (empty($post->is_new)) {
      $post->is_new = FALSE;
      $wrapper = entity_metadata_wrapper('harmony_post', $post);

      if ($wrapper->field_harmony_thread->value()) {
        // Prevent users from editing within a locked thread.
        if ($wrapper->field_harmony_thread->locked->value()) {
          drupal_access_denied();
        }
      }
    }

    // Basic post information.
    // These elements are just values so they are not even sent to the client.
    foreach (array('post_id', 'uid', 'is_new') as $key) {
      $form[$key] = array(
        '#type' => 'value',
        '#value' => isset($post->$key) ? $post->$key : NULL,
      );
    }

    $form['title'] = array(
      '#type' => 'value',
      '#title' => t('Post title'),
      '#default_value' => $post->title,
    );

    $langcode = entity_language('harmony_post', $post);
    field_attach_form('harmony_post', $post, $form, $form_state, $langcode);

    $text_lang = $form['field_harmony_text']['#language'];
    $form['field_harmony_text'][$text_lang][0]['#title_display'] = 'invisible';
    $form['field_harmony_text'][$text_lang][0]['#attributes']['placeholder'] = t('Type here.');

    // Hide the thread entity reference field.
    $form['field_harmony_thread']['#access'] = FALSE;
    // Hide the "Post is reply to" entity reference field.
    $form['field_harmony_post_is_reply_to']['#access'] = FALSE;

    return $form;
  }

  /**
   * Overrides EntityInlineEntityFormController::entityFormSubmit().
   */
  public function entityFormSubmit(&$form, &$form_state) {
    parent::entityFormSubmit($form, $form_state);

    $post = $form['#entity'];
    $wrapper = entity_metadata_wrapper('harmony_post', $post);

    if ($post->is_new) {
      $wrapper->first_post->set(TRUE);
    }
    $wrapper->title->set($form_state['values']['title']);
    $wrapper->status->set($form_state['values']['status']);
    $wrapper->locked->set($form_state['values']['locked']);

    field_attach_submit('harmony_post', $post, $form, $form_state);
  }

  /**
   * Overrides EntityInlineEntityFormController::save().
   */
  public function save($entity, $context) {
    harmony_post_save($entity);
  }
}
