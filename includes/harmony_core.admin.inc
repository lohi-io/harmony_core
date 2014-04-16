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
