<?php
/**
 * @file
 * harmony_core.tokens.inc
 */

/**
 * Implements hook_token_info().
 */
function harmony_core_token_info() {
  // Just tack an extra token in, entity_token will be doing the rest.
  $info = array(
    'tokens' => array(
      'harmony_thread' => array(
        'summary' => array(
          'name' => t('Summary'),
          'description' => t("The summary of the thread's first post text."),
        ),
      ),
    ),
  );

  // Integrate with the profanity module.
  if (module_exists('profanity')) {
    $wordlists = profanity_get_lists_flat();
    if (!empty($wordlists)) {
      $lists = array();
      foreach ($wordlists as $name => $label) {
        $lists[] = check_plain("$label ($name)");
      }
      $lists_replacement = t('Possible lists: !lists', array('!lists' => implode(', ', $lists)));

      $info['tokens']['harmony_thread']['summary-profanity'] = array(
        'name' => t('Summary profanity filtered'),
        'description' => t("The summary of the thread's first post text processed by a specified profanity word list. Use with the word list machine name. !replacements", array('!replacements' => $lists_replacement)),
        'dynamic' => TRUE,
      );
    }
  }

  return $info;
}

/**
 * Implements hook_tokens().
 */
function harmony_core_tokens($type, $tokens, array $data = array(), array $options = array()) {
  if (isset($options['language'])) {
    $url_options['language'] = $options['language'];
    $language_code = $options['language']->language;
  }
  else {
    $language_code = NULL;
  }
  $sanitize = !empty($options['sanitize']);
  $replacements = array();

  if ($type == 'harmony_thread' && !empty($data['harmony_thread'])) {
    $thread = $data['harmony_thread'];
    $wrapper = entity_metadata_wrapper($type, $thread);

    foreach ($tokens as $name => $original) {
      if ($name === 'summary') {
        if ($wrapper->field_harmony_first_post->value() && $wrapper->field_harmony_first_post->field_harmony_text->value()) {
          $text = $wrapper->field_harmony_first_post->field_harmony_text->raw();

          if ($sanitize) {
            $sanitised_text = check_plain($text['value']);
            $trimmed_text = truncate_utf8($sanitised_text, variable_get('harmony_core_meta_summary_max_length', 200), FALSE, TRUE);
            $replacements[$original] = $trimmed_text;
          }
          else {
            $replacements[$original] = $text['value'];
          }
        }
      }
      elseif (strpos($name, 'summary-profanity') === 0) {
        if (module_exists('profanity') && $wrapper->field_harmony_first_post->value() && $wrapper->field_harmony_first_post->field_harmony_text->value()) {
          $name_parts = explode(':', $name);
          $text = $wrapper->field_harmony_first_post->field_harmony_text->raw();

          if ($sanitize) {
            $sanitised_text = check_plain($text['value']);
            $trimmed_text = truncate_utf8($sanitised_text, variable_get('harmony_core_meta_summary_max_length', 200), FALSE, TRUE);
            $replacements[$original] = $trimmed_text;
          }
          else {
            $replacements[$original] = $text['value'];
          }

          // Profanity filter the content.
          if (!empty($name_parts[1])) {
            $replacements[$original] = profanity_list_execute($name_parts[1], $replacements[$original]);
          }
        }
      }
    }
  }

  return $replacements;
}
