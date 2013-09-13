<?php

/**
 * @file
 * harmony_core.context.inc
 * Code for integrating with Context.
 */

/**
 * Expose thread view as a context condition.
 */
class harmony_core_context_condition_thread extends context_condition {
  function thread_properties() {
    return array(
      'all' => array(
        'title' => t('All threads'),
      ),
      'status_true' => array(
        'title' => t('Published threads'),
        'property' => 'status',
        'value' => THREAD_PUBLISHED,
      ),
      'status_false' => array(
        'title' => t('Unpublished threads'),
        'property' => 'status',
        'value' => THREAD_NOT_PUBLISHED,
      ),
      'pinned_true' => array(
        'title' => t('Pinned threads'),
        'property' => 'pinned',
        'value' => THREAD_PINNED,
      ),
      'pinned_false' => array(
        'title' => t('Not pinned threads'),
        'property' => 'pinned',
        'value' => THREAD_NOT_PINNED,
      ),
      'locked_true' => array(
        'title' => t('Locked threads'),
        'property' => 'locked',
        'value' => THREAD_LOCKED,
      ),
      'locked_false' => array(
        'title' => t('Not locked threads'),
        'property' => 'locked',
        'value' => THREAD_NOT_LOCKED,
      ),
    );
  }

  function condition_values() {
    $values = array();

    foreach ($this->thread_properties() as $k => $property) {
      $values[$k] = $property['title'];
    }

    return $values;
  }

  function execute($thread) {
    $properties = $this->thread_properties();

    foreach ($this->get_contexts() as $context) {
      $conditions = $context->conditions['thread']['values'];

      foreach ($conditions as $condition) {
        if ($condition == 'all' || isset($thread->{$properties[$condition]['property']}) && $thread->{$properties[$condition]['property']} == $properties[$condition]['value']) {
          $this->condition_met($context, $condition);
          break;
        }
      }
    }
  }
}
