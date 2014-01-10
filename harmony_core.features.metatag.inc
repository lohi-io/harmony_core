<?php
/**
 * @file
 * harmony_core.features.metatag.inc
 */

/**
 * Implements hook_metatag_export_default().
 */
function harmony_core_metatag_export_default() {
  $config = array();

  // Exported Metatag config instance: harmony_post.
  $config['harmony_post'] = array(
    'instance' => 'harmony_post',
    'config' => array(
      'title' => array(
        'value' => 'Post in [harmony_post:field-harmony-thread:title] by [harmony_post:uid:name] | [site:name]',
      ),
    ),
  );

  // Exported Metatag config instance: harmony_thread.
  $config['harmony_thread'] = array(
    'instance' => 'harmony_thread',
    'config' => array(
      'title' => array(
        'value' => '[harmony_thread:title] | [site:name]',
      ),
    ),
  );

  return $config;
}
