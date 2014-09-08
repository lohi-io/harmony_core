<?php

/**
 * @file
 * Handler for number of replies to a thread.
 */

class harmony_core_views_handler_thread_field_replies extends views_handler_field {
  function option_definition() {
    $options = parent::option_definition();

    // These defaults will be passed through t() eventually.
    $options['singular'] = array(
      'default' => 'reply',
    );
    $options['plural'] = array(
      'default' => 'replies',
    );
    $options['short_scale_formatter'] = array(
      'default' => FALSE,
      'bool' => TRUE,
    );

    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['singular'] = array(
      '#type' => 'textfield',
      '#title' => t('Single reply text'),
      '#required' => TRUE,
      '#default_value' => $this->options['singular'],
      '#description' => t('The text to be used when there is a single reply to the thread.'),
    );

    $form['plural'] = array(
      '#type' => 'textfield',
      '#title' => t('Multiple replies text'),
      '#required' => TRUE,
      '#default_value' => $this->options['plural'],
      '#description' => t('The text to be used when there is either no replies or more than one.'),
    );

    if (module_exists('short_scale_formatter')) {
      $form['short_scale_formatter'] = array(
        '#title' => t('Output the number with the Short scale formatter module'),
        '#type' => 'checkbox',
        '#default_value' => $this->options['short_scale_formatter'],
      );
    }
  }

  /*function render_item($count, $item) {
    // Monkey with the number that will be rendered, reduce by 1.
    //$item['rendered']['#number']--;
    //$item['rendered']['#formatted']--;

    // Append our text.
    $text = $item['raw']['value'] - 1 == 1 ? check_plain($this->options['singular']) : check_plain($this->options['plural']);


    return render($item['rendered']) . ' ' . t($text);
  }*/

    /**
   * Get the value that's supposed to be rendered.
   *
   * This api exists so that other modules can easy set the values of the field
   * without having the need to change the render method as well.
   *
   * @param $values
   *   An object containing all retrieved values.
   * @param $field
   *   Optional name of the field where the value is stored.
   */
  function get_value($values, $field = NULL) {
    $alias = isset($field) ? $this->aliases[$field] : $this->field_alias;
    if (isset($values->{$alias})) {
      return $values->{$alias} - 1;
    }
  }

  /**
   * Render the field.
   *
   * @param $values
   *   The values retrieved from the database.
   */
  function render($values) {
    $value = $this->get_value($values);
    $text = $value == 1 ? check_plain($this->options['singular']) : check_plain($this->options['plural']);

    /*if (module_exists('short_scale_formatter') && $this->options['short_scale_formatter']) {
    }*/

    // This is bad form on translating things but not sure on the better way.
    return $this->sanitize_value($value) . ' ' . t($text);
  }
}
