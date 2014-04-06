<?php

/**
 * @file
 * Definition of harmony_core_views_handler_area_add_thread.
 */

/**
 * Listing header, provides a header with a link to create a new
 * thread, appended with current category tid if present.
 *
 * @ingroup views_area_handlers
 */
class harmony_core_views_handler_area_add_thread extends views_handler_area {

  function option_definition() {
    $options = parent::option_definition();

    $options['link_text'] = array('default' => t('Create a thread'));

    return $options;
  }

  /**
   * Default options form that provides the label widget that all fields
   * should have.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['link_text'] = array(
      '#type' => 'textfield',
      '#title' => t('Link text'),
      '#default_value' => $this->options['link_text'],
      '#description' => t('The text that will be used as the link label text.'),
    );
  }

  /**
   * Render the area
   */
  function render($empty = FALSE) {
    if (!empty($this->options['link_text'])) {
      // @todo harmony_access stuff.
      if (user_access('create harmony_threads')) {
        // @todo this should grab tid value from view object.
        $attrs = array();
        if (arg(0) === 'taxonomy' && arg(1) === 'term' && is_numeric(arg(2))) {
          $attrs = array(
            'query' => array(
              'field_harmony_category' => arg(2),
            ),
          );
        }

        drupal_add_css(drupal_get_path('module', 'harmony_core') . '/css/harmony-core.css');
        return theme('harmony_listing_header', array('link_text' => $this->options['link_text'], 'link_attrs' => $attrs));
      }
    }

    return '';
  }
}