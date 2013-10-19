<?php

/**
 * @file
 * Provide additional Views fields for entity content.
 */

/**
 * Implements hook_views_data_alter().
 *
 * We need to use this hook instead of hook_views_data() as
 * entity API already provides data for our custom entity type.
 */ 
 function harmony_core_views_data_alter(&$data) {
  // Extra Threads fields.
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

  // ----------------------------------------------------------------------
  // History table

  // We're actually defining a specific instance of the table, so let's
  // alias it so that we can later add the real table for other purposes if we
  // need it.
  $data['thread_read_history_user']['moved to'] = 'thread_read_history';
  $data['thread_read_history']['table']['group']  = t('Thread');

  // Explain how this table joins to others.
  $data['thread_read_history']['table']['join'] = array(
     // Directly links to node table.
    'thread' => array(
      'table' => 'thread_read_history',
      'left_field' => 'thread_id',
      'field' => 'thread_id',
      'extra' => array(
        array('field' => 'uid', 'value' => '***CURRENT_USER***', 'numeric' => TRUE),
      ),
    ),
  );

  $data['thread_read_history']['timestamp'] = array(
    'title' => t('Thread new/updated'),
    'field' => array(
      'help' => t('Show a marker if the content is new or updated.'),
      'handler' => 'harmony_core_views_handler_field_thread_read_history_timestamp',
    ),
    'filter' => array(
      'help' => t('Show only content that is new or updated.'),
      'handler' => 'harmony_core_views_handler_filter_thread_read_history_timestamp',
    ),
  );

  // Extra Post entity fields.
  $data['post']['view_link'] = array(
    'field' => array(
      'title' => t('Link'),
      'help' => t('Provide a simple link to the post.'),
      'handler' => 'harmony_core_views_handler_post_field_link',
    ),
  );  
  $data['post']['edit_link'] = array(
    'field' => array(
      'title' => t('Edit link'),
      'help' => t('Provide a simple link to edit the post.'),
      'handler' => 'harmony_core_views_handler_post_field_link_edit',
    ),
  );
  $data['post']['delete_link'] = array(
    'field' => array(
      'title' => t('Delete link'),
      'help' => t('Provide a simple link to delete the post.'),
      'handler' => 'harmony_core_views_handler_post_field_link_delete',
    ),
  );

  // Override the title field slightly to use our custom views field handler
  // which has the "link to post" functionality.
  $data['post']['title']['field']['handler'] = 'harmony_core_views_handler_post_field';
}
