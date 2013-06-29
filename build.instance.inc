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

  // Check whether or not the site exists
  if (file_exists($alias_info['root'])) {
    // This is OK, if rebuild is specified
    if (!drush_get_option('rebuild')) {
      drush_set_error("DRUSH_INSTANCE_ALREADY_EXISTS", "Instance $alias already exists at " . $alias_info['root']);
    }
  }
  // If rebuild IS specified, but there's no existing build, raise error
  elseif (drush_get_option('rebuild')) {
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
  // Path specifications
  if (!array_key_exists('paths', $alias_info['instance'])) {
    $alias_info['instance']['paths'] = array();
  }
  // Rebuild-specific validation
  if (drush_get_option('rebuild')) {
    _drush_instance_build_validate_rebuild($alias_info);
  }

  // Save the site alias in context for use in the actual command
  drush_set_context("DRUSH_INSTANCE", $alias_info);
}

/**
 * Rebuild-specific validation
 */
function _drush_instance_build_validate_rebuild(&$alias_info) {
  if (!array_key_exists('preserve_on_rebuild', $alias_info['instance']['paths'])) {
    // Don't have any? Can we at least preserve %files?
    if(array_key_exists('path-aliases', $alias_info) 
       && array_key_exists('%files', $alias_info['path-aliases'])) {
      $alias_info['instance']['preserve_on_rebuild'] = array($alias_info['path-aliases']['%files']);
      drush_log("I won't rebuild a site without preserving %files unless you explicitly tell me to, so I've added that to the list now", "ok");
    }
    // Otherwise, just warn the user. This warning can be suppressed
    // by creating an empty array of preserve_on_rebuild files, thus
    // explicitly stating "no preserve_on_rebuild required"
    else {
      drush_log("There are no paths configured to preserve_on_rebuild: any non-Drupal paths INCLUDING FILES will be left behind.", "warning");
    }
  }
  // Replace any path aliases e.g. %files
  foreach($alias_info['instance']['paths']['preserve_on_rebuild'] as $k => $path) {
    if (array_key_exists($path, $alias_info['path-aliases'])) {
      $alias_info['instance']['paths']['preserve_on_rebuild'][$k]
        = $alias_info['path-aliases'][$path];
    }
  }
  // And now check those files exist!
  foreach($alias_info['instance']['paths']['preserve_on_rebuild'] as $path) {
    if (!file_exists($alias_info['root'] . "/$path")) {
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
  $rebuild = (bool)drush_get_option('rebuild');

  // Try to find makefile
  drush_log("Looking for makefile", "notice");
  $master_tmp = make_tmp();
  $makefile_tmp = "$master_tmp/makefile";
  make_download_git(
    'Instance makefile',
    $instance['repos']['makefile'],
    $makefile_tmp
  );
  $makefile_location = "$makefile_tmp/" . $instance['makefile_path'];
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
  // Unset any of our options
  foreach(array('rebuild') as $option) {
    drush_unset_option($option);
  }
  drush_invoke('make', array($makefile_location, $site_tmp));

  // Replace sites/default with repository specified, defaulting to
  // the makefile repository (see validation hook)
  $sites_default = "$site_tmp/sites/default";
  drush_delete_dir($sites_default);
  make_download_git(
    'Instance makefile',
    $instance['repos']['sites_default'],
    $sites_default
  );
  drush_log("Replaced sites/default with repository", "ok");

  // Do the rest
  drush_log("TODO: sort out file permissions", "warning");
  drush_log("TODO: create a local-settings.php if specified", "warning");
  drush_log("TODO: render some templates: vhost, crontab etc.", "warning");
  drush_log("TODO: maybe rsync db (and files?) from another site alias", "warning");
  drush_log("TODO: anonymizing live db?", "warning");

  // Finally, move it to the site alias root
  // If this is a rebuild, then there will be an existing file root
  if($rebuild) {
    // Move it to one side and report success or failure
    $old_dir = $alias_info['root'] . '-old-' . rand(1, 99999);
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
  if ($rebuild) {
    foreach($instance['paths']['preserve_on_rebuild'] as $path) {
      $full_old_path = "$old_dir/$path";
      $full_new_path = $alias_info['root'] . "/$path";
      // Check all directories on new path exist, not including the last bit
      $check_path = "/";
      foreach(split("/", $full_new_path) as $new_path_part) {
        if(!file_exists($check_path) && !drush_mkdir($check_path)) {
          $some_paths_not_preserved = TRUE;
          drush_log("Could not create all subdirectories required to preserve $path", "warning");
          break 2;
        }
        $check_path .= "/$new_path_part";
      }
      // Again, to avoid replicating large files, we should use rename() - 
      // drush_move_dir() actually copies the contents
      if($ret) {
        $ret = @drush_op('rename', $full_old_path, $full_new_path);
      }
      if (!$ret) {
        $some_paths_not_preserved = TRUE;
        drush_log("Could not move $path to new build", "warning");
      }
      else {
        drush_log("Moved $path to new build", "ok");
      }
    }
  }

  if ($some_paths_not_preserved) {
    return drush_set_error("Your codebase has been rebuilt, but some paths could not be moved into it. It could therefore be in a partially built state, so please check the error log above.");
  }
  drush_log(dt("Finished !alias: deployed to !root",
            array("!alias" => $alias, "!root" => $alias_info['root'])),
            "ok");
}
