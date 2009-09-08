<?php
// $Id: apachesolr.drush.inc,v 1.1.2.1 2009/09/08 14:03:45 robertDouglass Exp $

/**
 * @file
 *   drush integration for apachesolr.
 */

/**
 * Implementation of hook_drush_command().
 *
 * In this hook, you specify which commands your
 * drush module makes available, what it does and
 * description.
 *
 * Notice how this structure closely resembles how
 * you define menu hooks.
 *
 * @See drush_parse_command() for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */
function apachesolr_drush_command() {
  $items = array();

  // the key in the $items array is the name of the command.
  $items['solr delete index'] = array(
    // the name of the function implementing your command.
    'callback' => 'apachesolr_drush_solr_delete_index',
    // a short description of your command
    'description' => "Deletes the content from the index. Can take content types as parameters.",
    'arguments' => array(
      'types' => 'Optional. A space delimited list of content types to be deleted from the index.',
    ),
  );
  $items['solr reindex'] = array(
    // the name of the function implementing your command.
    'callback' => 'apachesolr_drush_solr_reindex',
    // a short description of your command
    'description' => "Marks content for reindexing. Can take content types as parameters.",
    'arguments' => array(
      'types' => 'Optional. A space delimited list of content types to be marked for reindexing.',
    ),
  );
  return $items;
}

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function apachesolr_drush_help($section) {
  switch ($section) {
    case 'drush:solr delete index':
      return dt("Used without parameters, this command deletes the entire Solr index. Used with parameters for content type, it deletes just the content types that are specified. After the index has been deleted, all content will be indexed again on future cron runs.");
    case 'drush:solr reindex':
      return dt("Used without parameters, this command marks all of the content in the Solr index for reindexing. Used with paramters for content type, it marks just the content types that are specified. Reindexing is different than deleting as the content is still searchable while it is in queue to be reindexed. Reindexing is done on future cron runs.");
  }
}

/**
 * Example drush command callback.
 *
 * This is where the action takes place.
 *
 * In this function, all of Drupals API is (usually) available, including
 * any functions you have added in your own modules/themes.
 *
 * To print something to the terminal window, use drush_print().
 *
 */
function apachesolr_drush_solr_delete_index() {
  $args = func_get_args();
  $path = drupal_get_path('module', 'apachesolr');
  include_once($path . "/apachesolr.admin.inc");
  if (count($args) > 0) {
    foreach ($args as $type) {
      apachesolr_delete_index($type);
    }
  }
  else {
    apachesolr_delete_index();
  }
}

function apachesolr_drush_solr_reindex() {
  $args = func_get_args();
  if (count($args) > 0) {
    foreach ($args as $type) {
      apachesolr_rebuild_index_table($type);
    }
  }
  else {
    apachesolr_rebuild_index_table();
  }
}


