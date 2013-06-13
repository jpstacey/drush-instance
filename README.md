Drush instance
==============

Sitealias-driven drush-making of local development instances and live
production instances. Building an instance should only ever require:

* A sites/default repository
* A Drush makefile (which could go in the above)
* A Drush site alias

All of these can be stored under version control, but you never need to
store any of the d.o projects yourself, drastically reducing download sizes.

Rationale
---------

[Drush make][1] only gets you so far in building a site: it builds exactly
what's specified in the makefile but goes no further. What else does a
local site build need? Well:

* A sites/default containing your instance's custom code
* A relevant settings.php file
* Uploaded assets in your files directory (possibly synced from live)
* A database from somewhere (possibly ditto)
* Templates to use for e.g. vhost, crontab entries etc.

And if you want to redeploy your local instance, to clear out cruft or
switch to a completely different branch of your makefile, then at least
some of these need to be carried over from the previous deployment. Drush
instance should understand all of this "post-make" procedure and help you
implement it.

Related projects
----------------

* [Drush make][1] is as discussed great for kicking off a site build, but
doesn't fill in all the gaps.
* [Drush deploy][2] is a great Capistrano-like framework for remote
deployment of code and scripting of Drush tasks. That's not what this is
intended to be: it's meant to encourage use of Drush make and development
* [Aegir][3] is intended for moving/replicating site instances within a
server or from server to server. How you build the initial instance, and
how you store it under version control, is still up to you.

[1]: http://drupal.org/project/drush_make
[2]: http://drupal.org/project/drush_deploy
[3]: http://www.aegirproject.org/
