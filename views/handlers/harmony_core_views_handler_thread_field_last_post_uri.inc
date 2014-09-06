<?php

/**
 * @file
 * Contains the Last post URI field handler.
 */

/**
 * Field handler to provide simple renderer that allows linking to a thread.
 * Definition terms:
 * - link_to_thread default: Should this field have the checkbox "link to thread" enabled by default.
 *
 * @ingroup views_field_handlers
 */
class harmony_core_views_handler_thread_field_last_post_uri extends views_handler_field {
  /**
   * Render the field.
   *
   * @param $values
   *   The values retrieved from the database.
   */
  function render($values) {
    $value = $this->get_value($values);

    if ($value) {
      $value = substr(harmony_core_get_last_post_uri(unserialize($value)), 1);
    }

    return $this->sanitize_value($value);
  }
}
