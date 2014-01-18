<?php

/**
 * @file
 * Default theme implementation to display a Harmony Post entity.
 */

?>
<div id="post-<?php print $post->post_id; ?>" class="<?php print $classes; ?>"<?php print $attributes; ?>>
  <a name="post-<?php print $post->post_id; ?>"></a>
  <div class="post-header post-left-offset clearfix text-small">
    <span class="post-created pull-right"><?php print $created; ?></span>
  </div>
  <div class="clearfix">
    <div class="post-left post-user-profile text-center">
      <?php print $user_profile; ?>
    </div>
    <div class="post-content post-left-offset">
      <?php print render($content); ?>
    </div>
  </div>
</div>
