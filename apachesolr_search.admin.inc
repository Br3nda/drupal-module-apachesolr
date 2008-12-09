<?php
// $Id: apachesolr_search.admin.inc,v 1.1.2.1 2008/12/09 01:01:29 pwolanin Exp $

/**
 * @file
 *   Administrative settings for searching.
 */

/**
 * Menu callback - the settings form.
 */
function apachesolr_search_settings_page() {
  return drupal_get_form('apachesolr_search_settings_form');
}

/**
 * Form builder function to set query field weights.
 */
function apachesolr_search_settings_form() {
  $form = array();

  // get the current weights
  $qf = variable_get('apachesolr_search_query_fields', array());
  $weights = array();
  $weights['0'] = t('Omit');
  $weights += drupal_map_assoc(array('0.1', '0.2', '0.5', '1.0', '2.0', '3.0', '5.0', '8.0', '13.0', '21.0'));
  // Note - we have default values set in solrconfig.xml, which will operate when 
  // none are set.
  $defaults = array(
    'body' => '0.5',
    'title' => '13.0',
    'name' => '5.0',
    'taxonomy_names' => '2.0',
    'tags_h1' => '21.0',
    'tags_h2_h3' => '13.0',
    'tags_h4_h5_h6' => '5.0',
    'tags_inline' => '2.0',
    'tags_a' => '2.0',
  );
  if (!$qf) {
    $qf = $defaults;
  }
  // try to fetch the schema fields
  $solr = apachesolr_get_solr();
  $fields = $solr->getFields();
  if ($fields) {

    $form['apachesolr_search_query_fields'] = array(
      '#type' => 'fieldset',
      '#title' => t('Field weights'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#tree' => TRUE,
      '#description' => t('Specify here which fields are more important when searching. Give a field a bigger numeric value to make it more important.  If you omit a field, it will not be searched for keywords.'),
    );
    // Make sure all the default fields are included, even if they have no indexed content.
    foreach ($defaults as $field_name => $weight) {
      $form['apachesolr_search_query_fields'][$field_name] = array(
        '#type' => 'select',
        '#options' => $weights,
        '#title' => t('Weight for %field_name', array('%field_name' => $field_name)),
        '#default_value' => isset($qf[$field_name]) ? $qf[$field_name] : $defaults[$field_name],
      );
    }
    foreach ($fields as $field_name => $field) {
      $form['apachesolr_search_query_fields'][$field_name] = array(
        '#access' => $field->type == 'text',
        '#type' => 'select',
        '#options' => $weights,
        '#title' => t('Weight for %field_name', array('%field_name' => $field_name)),
        '#default_value' => isset($qf[$field_name]) ? $qf[$field_name] : '0',
      );
    }

    ksort($form['apachesolr_search_query_fields']);
  }

  return system_settings_form($form);
}

