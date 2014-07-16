<?php

/**
 * @file
 * Definition of harmony_core_views_handler_thread_field_link.
 */

/**
 * Field handler to present a link to the thread's reply form.
 *
 * @ingroup views_field_handlers
 */
class harmony_core_views_handler_thread_field_reply_link extends harmony_core_views_handler_thread_field_link {
  function render_link($thread, $values) {
    // Bail out if the user can't even see the thread.
    if (!harmony_core_thread_access_callback('view', $thread)) {
      return;
    }

    $this->options['alter']['make_link'] = TRUE;

    // Output the link if the user has access, otherwise provide a link
    // via the login page.
    if (harmony_core_thread_access_callback('create', $thread)) {
      $this->options['alter']['path'] = url('post/add', array(
        'query' => array(
          'field_harmony_thread' => $thread->thread_id,
        ),
      ));
      $this->options['alter']['link_class'] = 'reply-link';
    }
    else {
      $this->options['alter']['path'] = url('user', array(
        'query' => array(
          'destination' => url('post/add', array(
            'query' => array(
              'field_harmony_thread' => $thread->thread_id,
            ),
          )),
        ),
      ));
      $this->options['alter']['link_class'] = 'login-link';
    }

    return !empty($this->options['text']) ? check_plain($this->options['text']) : t('reply');
  }
}
