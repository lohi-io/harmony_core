<?php

/**
 * @file
 * Provide Harmony specific Rules actions and events.
 */

/**
 * Implements hook_rules_action_info().
 */
function harmony_core_rules_action_info() {
  return array(
    'harmony_thread_meta_save' => array(
      'label' => t('Thread meta save'),
      'parameter' => array(
        'thread' => array(
          'type' => 'harmony_thread',
          'label' => t('Thread'),
        ),
      ),
      'group' => t('Harmony'),
    ),
    'harmony_post_meta_save' => array(
      'label' => t('Post meta save'),
      'parameter' => array(
        'thread' => array(
          'type' => 'harmony_post',
          'label' => t('Post'),
        ),
      ),
      'group' => t('Harmony'),
    ),
  );
}
