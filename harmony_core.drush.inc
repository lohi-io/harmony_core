<?php

/**
 * @file
 * drush integration for Harmony core.
 */

/**
 * Implements hook_drush_command().
 */
function harmony_core_drush_command() {
  $items = array();

  $items['harmony-rebuild-thread-post-uris'] = array(
    'callback' => 'harmony_core_drush_rebuild_thread_post_uris',
    'description' => dt('Rebuilds the thread last post uri property on all threads.'),
  );

  /*$items['harmony-rebuild-counts'] = array(
    'callback' => 'harmony_core_drush_rebuild_counts',
    'description' => dt('Rebuilds all of the thread and posts counts.'),
  );

  $items['harmony-rebuild-thread-meta'] = array(
    'callback' => 'harmony_core_drush_rebuild_thread_meta',
    'description' => dt('Rebuilds all of the thread meta data (Post URIs, First and Last thread entity references and Last user to update).'),
  );*/

  /*$items['harmony-rebuild-thread-first-last'] = array(
    'callback' => 'harmony_core_drush_rebuild_thread_first_last',
    'description' => dt('Rebuilds the thread first and last entity references on all threads.'),
  );*/

  return $items;
}

/*function harmony_core_drush_rebuild_counts() {

}

function harmony_core_drush_rebuild_thread_meta() {

}*/

function harmony_core_drush_rebuild_thread_post_uris() {
  module_load_include('module', 'harmony_core', 'harmony_core');
  harmony_core_admin_rebuild_execute('last_post_uris');
  drush_backend_batch_process();
}

/*function harmony_core_drush_rebuild_thread_first_last() {

}*/
