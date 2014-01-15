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

/**
 * Processes variables for harmony-post.tpl.php
 *
 * The $variables array contains the following arguments:
 * - $post
 * - $view_mode
 * - $page
 *
 * @see harmony-post.tpl.php
 */
function template_preprocess_harmony_post(&$variables) {
  $variables['view_mode'] = $variables['elements']['#view_mode'];
  $variables['post'] = $variables['elements']['#post'];
  $post = $variables['post'];

  // Add in some useful variables.
  $variables['title'] = check_plain($post->title);
  $variables['url'] = url("post/$post->post_id");
  $variables['created'] = format_date($post->created);
  $variables['updated'] = format_date($post->updated);

  // Flatten the post object's member fields.
  $variables = array_merge((array) $post, $variables);

  // Helpful $content variable for templates.
  $variables += array('content' => array());
  foreach (element_children($variables['elements']) as $key) {
    $variables['content'][$key] = $variables['elements'][$key];
  }

  // Make the field variables available with the appropriate language.
  field_attach_preprocess('harmony_post', $post, $variables['content'], $variables);

  // Add in the post classes.
  $variables['classes_array'][] = 'post';
  $variables['classes_array'][] = 'post-' . drupal_html_class($variables['view_mode']);
  if (!$variables['status']) {
    $variables['classes_array'][] = 'post-unpublished';
  }
  if ($variables['locked']) {
    $variables['classes_array'][] = 'post-locked';
  }
  if ($variables['first_post']) {
    $variables['classes_array'][] = 'post-first-in-thread';
  }
  if (!empty($post->field_harmony_post_is_reply_to)) {
    $variables['classes_array'][] = 'post-is-direct-reply';
  }
  // Add in clearfix.
  $variables['classes_array'][] = 'clearfix';

  // Suggest some templates, including default? What's going wrong here?
  $variables['theme_hook_suggestions'][] = 'harmony_post';
  $variables['theme_hook_suggestions'][] = 'harmony_post__' . $post->post_id;
}
