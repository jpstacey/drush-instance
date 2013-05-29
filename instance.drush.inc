<?php
/**
 * @file
 *
 */

/**
 * Ipmlementation of hook_drush_help().
 */
function instance_drush_help($section) {
}

/**
 * Implementation of hook_drush_command().
 */
function instance_drush_command() {
  $items = array();
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
 * Implements drush_COMMAND_validate().
 */
function drush_instance_build_validate($alias) {
  if(!($alias_info = drush_sitealias_get_record($alias))) {
    drush_set_error("DRUSH_INSTANCE_SITEALIAS_DOES_NOT_EXIST", "Alias $alias does not exist");
  }
  elseif (array_key_exists('remote-host', $alias_info)) {
    drush_set_error("DRUSH_INSTANCE_SITEALIAS_REMOTE", "Sorry, drush instance does not currently support remote hosts like $alias.");
  }

  drush_set_context("DRUSH_INSTANCE", $alias_info);
}

/**
 * Implements drush_hook().
 */
function drush_instance_build($alias) {
  $alias_info = drush_get_context("DRUSH_INSTANCE");
  drush_log("Finished $alias!", "ok");
}
