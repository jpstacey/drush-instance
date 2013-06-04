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
    drush_set_error("DRUSH_INSTANCE_SITEALIAS_NONE", "Alias $alias does not exist");
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
    drush_set_error("DRUSH_INSTANCE_GIT_REPO_MAKEFILE", "No makefile git repository specified for $alias: you should specify \$alias['instance']['repos']['makefile'] as a makefile-like git repository definition");
  }

  // Check whether or not the site exists
  if (file_exists($alias_info['root'])) {
    drush_set_error("DRUSH_INSTANCE_ALREADY_EXISTS", "Instance $alias already exists at " . $alias_info['root']);
  }

  // If there's an error by this point, we shouldn't continue
  if (drush_get_error() == DRUSH_FRAMEWORK_ERROR) {
    return;
  }

  // Defaults
  // Makefile name
  if (!array_key_exists('makefile_path', $alias_info['instance'])) {
    $alias_info['instance']['makefile_path'] = 'instance.make';
  }
  // If there's no extra git repository for sites/default, assume it's the
  // same as the makefile repository
  if (!array_key_exists('sites_default', $alias_info['instance']['repos'])) {
    $alias_info['instance']['repos']['sites_default'] =
      $alias_info['instance']['repos']['makefile'];
  }

  // Save the site alias in context for use in the actual command
  drush_set_context("DRUSH_INSTANCE", $alias_info);
}

/**
 * Implements drush_hook().
 */
function drush_instance_build($alias) {
  $alias_info = drush_get_context("DRUSH_INSTANCE");

  // Check to see if site exists
  drush_log("TODO: ask if we want to delete the existing site [y/n]", "warning");

  // Try to find makefile
  drush_log("Looking for makefile", "notice");
  $master_tmp = make_tmp();
  $makefile_tmp = "$master_tmp/makefile";
  make_download_git(
    'Instance makefile',
    $alias_info['instance']['repos']['makefile'],
    $makefile_tmp
  );
  $makefile_location = "$makefile_tmp/" . $alias_info['instance']['makefile_path'];
  if(!file_exists($makefile_location)) {
    return drush_set_error("DRUSH_INSTANCE_MAKEFILE_NONE", "Makefile not present in repository.");
  }

  // Make site
  drush_log("Making site", "notice");
  // Drush make uses make_tmp() and doesn't play nicely (directory name
  // collisions, and it seems it deletes everything when it's done)
  // So generate our own temporary location
  $site_tmp = drush_find_tmp() . "/drush_instance_tmp_" . time() . '_' . uniqid();
  if (!drush_get_option('no-clean', FALSE)) {
    drush_register_file_for_deletion($site_tmp);
  }
  drush_invoke('make', array($makefile_location, $site_tmp));


  // Replace sites/default with repository specified, defaulting to
  // the makefile repository (see validation hook)
  $sites_default = "$site_tmp/sites/default";
  drush_delete_dir($sites_default);
  make_download_git(
    'Instance makefile',
    $alias_info['instance']['repos']['sites_default'],
    $sites_default
  );
  drush_log("Replaced sites/default with repository", "ok");

  // Do the rest
  drush_log("TODO: move over any specified files or file aliases, including %files etc.", "warning");
  drush_log("TODO: sort out file permissions", "warning");
  drush_log("TODO: create a local-settings.php if specified", "warning");
  drush_log("TODO: render some templates: vhost, crontab etc.", "warning");
  drush_log("TODO: maybe rsync db (and files?) from another site alias", "warning");
  drush_log("TODO: anonymizing live db?", "warning");

  // Finally, move it to the site alias root
  $ret = drush_move_dir($site_tmp, $alias_info['root'], FALSE);
  if (!$ret) {
    drush_set_error("Cannot move instance into site alias location");
  }

  drush_log("Finished $alias!", "ok");
}
