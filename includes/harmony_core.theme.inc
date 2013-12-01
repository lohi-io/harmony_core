<?php

/**
 * @file
 * harmony_core.theme.inc
 * Base theme functions for Harmony.
 */

/**
 * Display inline diff metadata.
 */
function theme_harmony_post_diff_inline_metadata($vars) {
  $post = $vars['post'];

  $output = "<div class='diff-inline-metadata clear-block'>";
  $output .= '<div class="diff-inline-legend clear-block">';
  $output .= '<label>' . t('Legend') . '</label>';
  $output .= theme('diff_inline_chunk', array('text' => t('Added'), 'type' => 'add'));
  $output .= theme('diff_inline_chunk', array('text' => t('Changed'), 'type' => 'change'));
  $output .= theme('diff_inline_chunk', array('text' => t('Deleted'), 'type' => 'delete'));
  $output .= '</div>';
  $output .= '</div>';

  return $output;
}
