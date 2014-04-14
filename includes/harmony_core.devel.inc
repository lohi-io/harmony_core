<?php

/**
 * @file
 * harmony_core.devel.inc
 *
 * Page and callbacks for devel tabs and generate stuff.
 * Code has been gleaned from devel_entity.
 */

function harmony_core_entity_load_object($entity_type, $entity, $name) {
  module_load_include('inc', 'entity', 'includes/entity.form');

  return kdevel_print_object($entity);
}

function harmony_core_render_object($entity_type, $entity, $name) {
  module_load_include('inc', 'entity', 'includes/entity.form');
  $build = $entity->view();

  return kdevel_print_object($build);
}
