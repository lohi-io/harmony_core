<?php

/**
 * @file
 * Default theme implementation to display a Harmony Post entity.
 */

hide($content['links']);
$links = render($content['links']);
?>
<div id="post-<?php print $post->post_id; ?>" class="<?php print $classes; ?>"<?php print $attributes; ?>>
  <a name="post-<?php print $post->post_id; ?>"></a>
  <div class="post-header post-left-offset clearfix text-small">
    <?php if ($direct_reply): ?>
    <div class="pull-left">
      <span class="post-is-reply-to">
        <?php print $direct_reply; ?>
      </span>
    </div>
    <?php endif; ?>
    <div class="pull-right">
      <span class="post-created"><?php print $created; ?></span>
      <?php if ($revisions): ?>
       <span class="post-revisions"><?php print $revisions; ?></span>
      <?php endif; ?>
    </div>
  </div>
  <div class="clearfix">
    <div class="post-left post-user-profile text-center">
      <?php print $user_profile; ?>
    </div>
    <div class="post-content post-left-offset">
      <?php print render($content); ?>

      <?php if ($links): ?>
      <div class="clearfix">
        <?php print $links; ?>
      </div>
      <?php endif; ?>
    </div>
  </div>
</div>
