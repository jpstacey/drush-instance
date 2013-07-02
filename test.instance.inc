<?php

/**
 * @file test.instance.inc
 * Test the build scripts
 */

/**
 * Implements drush_hook().
 */
function drush_instance_test() {
  // Include our test aliases
  drush_set_context('ALIAS_PATH', array(dirname(__FILE__) . '/tests'));

  // Bring in @instance.local, but fiddle with its root directory
  drush_sitealias_get_record('@instance.local');
  $aliases =& drush_get_context('site-aliases');
  $aliases['@instance.local']['root'] = $site_root = _drush_instance_tmp_unique();

  drush_log("RUN : build", "ok");
  // Now invoke instance-build
  drush_invoke('instance-build', array('@instance.local'));

  drush_log("TEST: build", "ok");
  // Tests:
  // 1. Drupal 7.21 downloaded
  if (file_exists("$site_root/CHANGELOG.txt") &&
     strpos(file_get_contents("$site_root/CHANGELOG.txt"),"Drupal 7.21, 2013-03-06") !== false) {
    drush_log("Found Drupal 7.21 CHANGELOG.txt", "success");
  }
  else {
    return drush_log("Cannot find Drupal 7.21 CHANGELOG.txt", "error");
  }
  // 2. sites/default is git repos and contains what we expect
  if (count(glob("$site_root/sites/default/instance.drush.inc"))) {
    drush_log("Found contents of sites/default", "success");
  }
  else {
    return drush_log("Cannot find example file in sites/default", "error");
  }

  // 3. sites/default has the correct git origin repository
  drush_shell_cd_and_exec("$site_root/sites/default", 'git remote show origin 2>' . drush_bit_bucket());
  $output = drush_shell_exec_output();
  foreach($output as $line) {
    if(trim($line) == "Fetch URL: git@github.com:jpstacey/drush-instance.git") {
      $found_origin = TRUE;
      drush_log("Found origin of sites/default git repository", "success");
    }
  }
  if (empty($found_origin)) {
    return drush_log("Could not find origin of sites/default git repository", "error");
  }

  // 4. sites/default is on the right branch
  drush_shell_cd_and_exec("$site_root/sites/default", 'git branch 2>' . drush_bit_bucket());
  $output = drush_shell_exec_output();
  if (!empty($output) && $output[0] == "* master") {
    drush_log("sites/default git repository is on master", "success");
  }
  else {
    return drush_log("Could not detect sites/default git repository to be on master", "error");
  }

  // Now add some files for the instance-rebuild
  file_put_contents("$site_root/some_random_toplevel_file.txt", 'test');
  drush_mkdir("$site_root/sites/files/core");

  // Now invoke instance-build with --rebuild set
  drush_log("RUN : rebuild", "ok");
  drush_set_option('rebuild', TRUE);
  // Have to get the temp directory recreated too: drush make is overzealous in deletion
  drush_mkdir(make_tmp());
  drush_mkdir(make_tmp() . '/__build__');
  drush_invoke('instance-build', array('@instance.local'));

  drush_log("TEST: rebuild", "ok");
  drush_set_error("Need to test that preserve_on_rebuild paths are indeed preserved!");
}
