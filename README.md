# Harmony core

## Features, things of note
+ Thread & Post entities along with fields supplied by Features
+ This module makes the assumptions that the following don't exist and that you're installing on to a vanilla install (hey, this is the core of a distro!) Taxonomy vocabs, Flags.
+ Anonymous cannot create threads or posts

## Frequently asked questions

**I'd like everything at /forum, how can I do that?**
Easily! The listing pages are views so you can change the paths on the page displays, and as for Categories and Threads you can change the URL alias patterns at: Configuration > Search and metadata > URL aliases > Patterns. This page and functionality is provided by the pathauto module which harmony_core requires.

## Language notes
As I'm British, that's the language I've used. Being mindful of those who'd rather have American-English here's a list of words that I've used, to come back and offer some kind of localisation around.
+ Colour

## Harmony core has integration with...

+ Pathauto module (thread entity)
+ Context (Trigger context when viewing a thread with x)
+ Views of course!
+ Inline entity form
+ Actions & Views bulk operations
+ Metatag module, thread & post.

## How do I use it?

You can add new threads here:
/thread/add

The default listing of posts can be found here:
/forum

You can add and alter fields on the Thread entity here:
/admin/structure/harmony/thread

And post here:
/admin/structure/harmony/post

## Install notes

jQuery update should be set to use 1.7, there are issues with this however, check the settings here:
/admin/config/development/jquery_update

If you want to use At.js you will need to download libraries, I refer you to the instructions for that module.

