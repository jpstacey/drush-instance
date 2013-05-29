<?php
/**
 * @file
 *
 */

/**
 * Imlpementation of hook_drush_help().
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
  // Check alias exists
  if(!($alias_info = drush_sitealias_get_record($alias))) {
    drush_set_error("DRUSH_INSTANCE_SITEALIAS_DOES_NOT_EXIST", "Alias $alias does not exist");
  }
  // Check it's a local alias
  elseif (array_key_exists('remote-host', $alias_info)) {
    drush_set_error("DRUSH_INSTANCE_SITEALIAS_REMOTE", "Sorry, drush instance does not currently support remote hosts like $alias.");
  }
  // Check it has a git repository for the makefile
  elseif (!(array_key_exists('instance', $alias_info)
            && array_key_exists('repos', $alias_info['instance'])
            && array_key_exists('makefile', $alias_info['instance']['repos'])
         )) {
    drush_set_error("DRUSH_INSTANCE_GIT_REPO_MAKEFILE", "No makefile git repository specified for $alias: you should specify \$alias['instance']['repos']['makefile']");
  }

  // If there's an error by this point, we shouldn't continue
  if (drush_get_error() == DRUSH_FRAMEWORK_ERROR) {
    return;
  }

  // If there's no extra git repository for sites/default, assume it's the
  // same as the makefile repository
  if (!array_key_exists('sites_default', $alias_info['instance']['repos'])) {
    $alias_info['instance']['repos']['sites_default'] =
      $alias_info['instance']['repos']['makefile'];
  }

  drush_set_context("DRUSH_INSTANCE", $alias_info);
}

/**
 * Implements drush_hook().
 */
function drush_instance_build($alias) {
  $alias_info = drush_get_context("DRUSH_INSTANCE");

  $makefile_dir = make_download_git(
    'Instance makefile',
    array('url' => $alias_info['instance']['repos']['makefile']),
    'makefile'
  );

  drush_log("Makefile $makefile_dir", "ok");
  drush_log("Finished $alias!", "ok");
}
