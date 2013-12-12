<?php
/**
 * @file
 * harmony_core.context.inc
 */

/**
 * Implements hook_context_default_contexts().
 */
function harmony_core_context_default_contexts() {
  $export = array();

  $context = new stdClass();
  $context->disabled = FALSE; /* Edit this to true to make a default context disabled initially */
  $context->api_version = 3;
  $context->name = 'harmony_category_page';
  $context->description = 'Is active on category term pages.';
  $context->tag = 'harmony';
  $context->conditions = array(
    'taxonomy_term' => array(
      'values' => array(
        'harmony_category' => 'harmony_category',
      ),
      'options' => array(
        'term_form' => '0',
      ),
    ),
  );
  $context->reactions = array(
    'block' => array(
      'blocks' => array(
        'views-e968ac19225f2c0a7dcc9ca744c116bc' => array(
          'module' => 'views',
          'delta' => 'e968ac19225f2c0a7dcc9ca744c116bc',
          'region' => 'content',
          'weight' => '-10',
        ),
      ),
    ),
  );
  $context->condition_mode = 0;

  // Translatables
  // Included for use with string extractors like potx.
  t('Is active on category term pages.');
  t('harmony');
  $export['category_page'] = $context;

  return $export;
}
