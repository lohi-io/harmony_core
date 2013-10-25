<?php
/**
 * @file
 * harmony_core.features.inc
 */

/**
 * Implements hook_ctools_plugin_api().
 */
function harmony_core_ctools_plugin_api() {
  list($module, $api) = func_get_args();
  if ($module == "context" && $api == "context") {
    return array("version" => "3");
  }
  list($module, $api) = func_get_args();
  if ($module == "taxonomy_display" && $api == "taxonomy_display") {
    return array("version" => "1");
  }
}

/**
 * Implements hook_views_api().
 */
function harmony_core_views_api() {
  return array("api" => "3.0");
}
