<?php
/**
 * @file
 * harmony_core.features.filter.inc
 */

/**
 * Implements hook_filter_default_formats().
 */
function harmony_core_filter_default_formats() {
  $formats = array();

  // Exported format: Harmony Filtered.
  $formats['harmony_filtered'] = array(
    'format' => 'harmony_filtered',
    'name' => 'Harmony Filtered',
    'cache' => 1,
    'status' => 1,
    'weight' => 0,
    'filters' => array(
      'codefilter' => array(
        'weight' => -50,
        'status' => 1,
        'settings' => array(),
      ),
      'filter_markdown' => array(
        'weight' => -49,
        'status' => 1,
        'settings' => array(),
      ),
      'filter_htmlcorrector' => array(
        'weight' => -48,
        'status' => 1,
        'settings' => array(),
      ),
      'filter_html' => array(
        'weight' => -47,
        'status' => 1,
        'settings' => array(
          'allowed_html' => '<h1> <h2> <h3> <h4> <h5> <h6> <em> <strong> <code> <del> <img> <blockquote> <q> <cite> <sup> <sub> <p> <br> <ul> <ol> <li> <dl> <dt> <dd> <a> <b> <u> <i> <table> <tr> <td> <th> <tbody> <thead> <tfoot> <colgroup> <caption> <hr> <!-->',
          'filter_html_help' => 1,
          'filter_html_nofollow' => 0,
        ),
      ),
      'filter_autop' => array(
        'weight' => -45,
        'status' => 1,
        'settings' => array(),
      ),
      'filter_url' => array(
        'weight' => -44,
        'status' => 1,
        'settings' => array(
          'filter_url_length' => 72,
        ),
      ),
    ),
  );

  return $formats;
}
