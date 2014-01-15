<?php

/**
 * @file
 * Default theme implementation to display a Harmony Post entity.
 */

?>
<div id="post-<?php print $post->post_id; ?>" class="<?php print $classes; ?>"<?php print $attributes; ?>>
  <?php print render($content); ?>
</div>
