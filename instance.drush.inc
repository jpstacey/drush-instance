<?php

/**
 * @file instance.drush.inc
 */

/**
 * Implements hook_drush_help().
 */
function instance_drush_help($section) {
  switch($section) {
    case 'drush:instance-build':
      return dt("Build a site instance from a site alias");
    break;
  }
}

/**
 * Implements hook_drush_command().
 */
function instance_drush_command() {
  $items = array();
  $items['instance-test'] = array(
    'description' => 'Test instance-build works',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['instance-build'] = array(
    'description' => 'Build a site instance from a site alias',
    'arguments' => array(
      'alias' => 'Site alias for the local build target',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  return $items;
}

/**
 * Make a unique temporary folder to avoid between-command collisions
 */
function _drush_instance_tmp_unique() {
  // Drush make uses make_tmp() and doesn't play nicely (directory name
  // collisions, and it seems it deletes everything when it's done)
  // So generate our own temporary location
  $site_tmp = drush_find_tmp() . "/drush_instance_tmp_" . time() . '_' . uniqid();
  if (!drush_get_option('no-clean', FALSE)) {
    drush_register_file_for_deletion($site_tmp);
  }
  return $site_tmp;
}
