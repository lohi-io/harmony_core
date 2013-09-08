<?php

/**
 * @file
 * Provide additional Views fields for entity content.
 */
 
/**
 * Implements hook_views_data_alter().
 */ 
 function harmony_core_views_data_alter(&$data) {

  // Extra threads fields.
  $data['thread']['view_link'] = array(
    'field' => array(
      'title' => t('Link'),
      'help' => t('Provide a simple link to the thread.'),
      'handler' => 'harmony_core_views_handler_thread_field_link',
    ),
  );  
  $data['thread']['edit_link'] = array(
    'field' => array(
      'title' => t('Edit link'),
      'help' => t('Provide a simple link to edit the thread.'),
      'handler' => 'harmony_core_views_handler_thread_field_link_edit',
    ),
  );
  $data['thread']['delete_link'] = array(
    'field' => array(
      'title' => t('Delete link'),
      'help' => t('Provide a simple link to delete the thread.'),
      'handler' => 'harmony_core_views_handler_thread_field_link_delete',
    ),
  );

  // Override the title field slightly to use our custom views field handler
  // which has the "link to thread" functionality.
  $data['thread']['title']['field']['handler'] = 'harmony_core_views_handler_thread_field';
}