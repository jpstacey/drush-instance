<?php

$aliases['instance.local'] = array(
    'uri' => 'instance.local',
    'root' => '/var/www/drupal7-core',
    'path-aliases' => array(
      '%files' => 'sites/files/core',
    ),
    'instance' => array(
      'repos' => array(
        'makefile' => array(
          'url' => 'git@github.com:jpstacey/drush-instance.git',
          'branch' => 'examples',
        ),
        'sites_default' => array(
          'url' => 'git@github.com:jpstacey/drush-instance.git',
        ),
      ),
      'makefile_path' => 'core.make',

      'paths' => array(
        'preserve_on_rebuild' => array(
          '%files',
          'some_random_toplevel_file.txt',
        ),
      ),
    ),
);

