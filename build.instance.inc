<?php

/**
 * @file build.instance.inc
 * Build an instance using a site alias
 */

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

  // Add the build/rebuild status
  $alias_info['_is_rebuild'] = drush_get_option('rebuild');

  // Check whether or not the site exists
  if (file_exists($alias_info['root'])) {
    // This is OK, if rebuild is specified
    if (!$alias_info['_is_rebuild']) {
      drush_set_error("DRUSH_INSTANCE_ALREADY_EXISTS", "Instance $alias already exists at " . $alias_info['root']);
    }
  }
  // If rebuild IS specified, but there's no existing build, raise error
  elseif ($alias_info['_is_rebuild']) {
    drush_set_error("DRUSH_INSTANCE_ALREADY_EXISTS", "Instance $alias does not exist at " . $alias_info['root'] . "; cannot rebuild");
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

  // Validation of paths array
  // used to be only run during rebuild
  _drush_instance_build_validate_paths($alias_info);

  // Save the site alias in context for use in the actual command
  drush_set_context("DRUSH_INSTANCE", $alias_info);
}

/**
 * Paths-specific validation
 */
function _drush_instance_build_validate_paths(&$alias_info) {
  // Path specifications
  if (!array_key_exists('paths', $alias_info['instance'])) {
    $alias_info['instance']['paths'] = array();
    // If this is a rebuild, we at least try to preserve %files
    if ($alias_info['_is_rebuild']) {
      // Don't have any? Can we at least preserve %files?
      if(array_key_exists('path-aliases', $alias_info)
         && array_key_exists('%files', $alias_info['path-aliases'])) {
        $alias_info['instance']['paths'][$alias_info['path-aliases']['%files']]
          = array('preserve_on_rebuild' => TRUE);
        drush_log("I won't rebuild a site without preserving %files unless you explicitly tell me to, so I've added that to the list now", "ok");
      }
      // Otherwise, just warn the user. This warning can be suppressed
      // by creating an empty array of preserve_on_rebuild files, thus
      // explicitly stating "no preserve_on_rebuild required"
      else {
        drush_log("There are no paths configured to preserve_on_rebuild: any non-Drupal paths INCLUDING FILES will be left behind.", "warning");
      }
    }
  }

  // Replace any path aliases e.g. %files
  foreach($alias_info['instance']['paths'] as $path => $config) {
    if (array_key_exists($path, $alias_info['path-aliases'])) {
      $alias_info['instance']['paths'][$alias_info['path-aliases'][$path]]
        = $config;
      unset($alias_info['instance']['paths'][$path]);
    }
  }

  // And now check those files exist!
  foreach($alias_info['instance']['paths'] as $path => $config) {
    if ($config['preserve_on_rebuild'] && $alias_info['_is_rebuild']
        && !file_exists($alias_info['root'] . "/$path")) {
      drush_log("Cannot preserve on rebuild as does not exist: $path", "warning");
    }
  }
}

/**
 * Implements drush_hook().
 */
function drush_instance_build($alias) {
  // Get all the config we'll need first
  $alias_info = drush_get_context("DRUSH_INSTANCE");
  $instance = $alias_info['instance'];
  $rebuild = (bool)$alias_info['_is_rebuild'];

  // Try to find makefile
  drush_log("Looking for makefile", "ok");
  $master_tmp = make_tmp();
  $makefile_tmp = "$master_tmp/makefile";
  make_download_git(
    'Instance-makefile',
    $instance['repos']['makefile'],
    $makefile_tmp
  );
  $makefile_location = "$makefile_tmp/" . $instance['makefile_path'];
  if(!file_exists($makefile_location)) {
    return drush_set_error("DRUSH_INSTANCE_MAKEFILE_NONE", "Makefile not present in repository.");
  }

  // Make site
  drush_log("Making site", "ok");
  // Drush make uses make_tmp() and doesn't play nicely (directory name
  // collisions, and it seems it deletes everything when it's done)
  // So generate our own temporary location
  $site_tmp = _drush_instance_tmp_unique();

  // Unset any of our possible options
  foreach(array('rebuild') as $option) {
    drush_unset_option($option);
  }
  drush_invoke('make', array($makefile_location, $site_tmp));

  // Replace sites/default with repository specified, defaulting to
  // the makefile repository (see validation hook)
  $sites_default = "$site_tmp/sites/default";
  drush_delete_dir($sites_default);
  // Assume deployed codebase should always be a working copy, so hot-swap a
  // TRUE value into that drush_get_option before running make_download_git()
  $old_wc = drush_get_option('working-copy');
  drush_set_option('working-copy', TRUE);
  make_download_git(
    'Sites-default',
    $instance['repos']['sites_default'],
    $sites_default
  );
  // Swap the old option for working-copy back in
  drush_set_option('working-copy', $old_wc);
  drush_log("Replaced sites/default with repository", "ok");

  // Do the rest
  drush_log("TODO: sort out file permissions", "warning");
  drush_log("TODO: render some templates: vhost, crontab etc.", "warning");
  drush_log("TODO: maybe rsync db (and files?) from another site alias", "warning");
  drush_log("TODO: anonymizing live db?", "warning");

  // Finally, move it to the site alias root
  // If this is a rebuild, then there will be an existing file root
  if($rebuild) {
    // Move it to one side and report success or failure
    $old_dir = $alias_info['root'] . '-old-' . rand(1, 99999);
    $alias_info['_old_dir'] = $old_dir;
    $ret = drush_move_dir($alias_info['root'], $old_dir);
    if (!$ret) {
      return drush_set_error("Cannot move existing instance aside in order to rebuild");
    }
    drush_log("Moved existing instance aside to $old_dir", "ok");
  }

  // Move new build into place
  $ret = drush_move_dir($site_tmp, $alias_info['root'], FALSE);
  if (!$ret) {
    return drush_set_error("Cannot move instance into site alias location");
  }
  drush_log("Moved new instance in place at " . $alias_info['root'], "ok");

  // Now move all preserved files over: we do this as late
  // as possible to avoid e.g. moving large filetrees across partitions,
  // as we can assume that e.g. /var/www/* is all on one partition (?)
  // but the same can't be said for e.g. /tmp
  $path_success = TRUE;
  foreach($instance['paths'] as $path => $config) {
    $path_success = $path_success && _drush_instance_build_path($alias_info, $path, $config, $rebuild);
    if (!$path_success) {
      break;
    }
  }
  if (!$path_success) {
    return drush_set_error("Your codebase has been rebuilt, but some paths could not be moved into it. It could therefore be in a partially built state, so please check the error log above.");
  }
  drush_log(dt("Finished !alias: deployed to !root",
            array("!alias" => $alias, "!root" => $alias_info['root'])),
            "ok");
}

/**
 * Generic handler for all paths
 */
function _drush_instance_build_path($alias_info, $path, $config, $rebuild) {
  // Specific handler based on type, or 'rebuild' if that's appropriate
  $type = $rebuild ? 'rebuild' : $config['source']['type'];
  $handler = "_drush_instance_build_path_$type";
  if (!function_exists($handler)) {
    drush_set_error("No handler method exists for preserving paths of type=$type");
    return FALSE;
  }
  return $handler($alias_info, $path, $config);
}

/**
 * Specific handler for rebuild paths
 */
function _drush_instance_build_path_rebuild($alias_info, $path, $config) {
  // Only preserve paths marked as such
  if (!$config['preserve_on_rebuild']) {
    return TRUE;
  }

  // Shortcut variables for old and new paths
  $full_old_path = $alias_info['_old_dir'] . "/$path";
  $full_new_path = $alias_info['root'] . "/$path";

  // Ensure all directories above this one exist
  if(!_drush_instance_build_path_containing_dir($full_new_path)) {
    drush_log("Could not create all subdirectories required to preserve $path", "warning");
    return FALSE;
  }

  // Again, to avoid replicating large files, we should use rename() -
  // drush_move_dir() actually copies the contents
  $ret = @drush_op('rename', $full_old_path, $full_new_path);
  if (!$ret) {
    drush_log("Could not move $path to new build", "warning");
  }
  else {
    drush_log("Moved $path to new build", "ok");
  }
  return $ret;
}

/**
 * Specific handler for touch paths
 */
function _drush_instance_build_path_touch($alias_info, $path, $config) {
  // Shortcut variable for new path
  $full_new_path = $alias_info['root'] . "/$path";

  // Ensure all directories above this one exist
  if(!_drush_instance_build_path_containing_dir($full_new_path)) {
    drush_log("Could not create all subdirectories required to set up empty file $path", "warning");
    return FALSE;
  }

  return (@file_put_contents($full_new_path, '') !== FALSE);
}

/**
 * Specific handler for touch paths
 */
function _drush_instance_build_path_mkdir($alias_info, $path, $config) {
  // Shortcut variable for new path
  $full_new_path = $alias_info['root'] . "/$path";

  // Ensure all directories above this one exist
  if(!_drush_instance_build_path_containing_dir($full_new_path)) {
    drush_log("Could not create all subdirectories required to set up directory $path", "warning");
    return FALSE;
  }

  return @drush_mkdir($full_new_path);
}

/**
 * Helper function to create all the directories down to a new path, but
 * not including it
 */
function _drush_instance_build_path_containing_dir($full_new_path) {
  // Check all directories on new path exist, not including the last bit
  $check_path = "/";
  foreach(split("/", $full_new_path) as $new_path_part) {
    if(!file_exists($check_path) && !drush_mkdir($check_path)) {
      return FALSE;
    }
    $check_path .= "/$new_path_part";
  }
  return TRUE;
}
