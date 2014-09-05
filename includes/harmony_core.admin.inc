<?php

/**
 * @file
 * harmony_core.admin.inc
 * Page and form callbacks for administration of the Thread and Post entity types.
 */

/**
 * Page callback for the landing structure settings page.
 */
function harmony_core_harmony_structure_settings() {
  return '';
}

function harmony_core_thread_entity_form($form, &$form_state, $bundle = array(), $op = 'edit') {

}

function harmony_core_post_entity_form($form, &$form_state, $bundle = array(), $op = 'edit') {
  $form['text'] = array(
    '#markup' => '<p>' . t('Nothing to see here, why not manage some fields?') . '</p>',
  );

  return $form;
}

/**
 * Returns a system form for disabling new Harmony forum content.
 */
function harmony_core_admin_disable_new_content() {
  $form = array();

  $form['guidance_text'] = array(
    '#markup' => '<p>' . t('Disabling new content is intended as a temporary measure and allows you to prevent new forum content from being created. A typical reason for this might be a SPAM attack.') . '</p>',
  );

  $form['harmony_core_disable_new_content'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disable new content from being created'),
    '#description' => t('This will prevent users from creating new threads or posts regardless of permissions.'),
    '#default_value' => variable_get('harmony_core_disable_new_content', FALSE),
  );

  $form['warning'] = array(
    '#type' => 'fieldset',
    '#title' => t('Warning message'),
    '#states' => array(
      'visible' => array(
        ':input[id="edit-harmony-core-disable-new-content"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['warning']['harmony_core_disable_new_content_warning_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display a warning message to all users?'),
    '#default_value' => variable_get('harmony_core_disable_new_content_warning_enabled', FALSE),
  );

  $form['warning']['harmony_core_disable_new_content_warning_message'] = array(
    '#type' => 'textarea',
    '#title' => t('Warning message text'),
    '#description' => t('The text to display in the drupal set message warning to users without access to this page.'),
    '#default_value' => variable_get('harmony_core_disable_new_content_warning_message', 'The creation of new threads and posts has been temporarily disabled.'),
    '#states' => array(
      'visible' => array(
        ':input[id="edit-harmony-core-disable-new-content-warning-enabled"]' => array('checked' => TRUE),
      ),
    ),
  );

  return system_settings_form($form);
}
