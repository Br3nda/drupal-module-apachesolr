<?php
// $Id: apachesolr_search.admin.inc,v 1.1.2.5 2008/12/16 05:33:10 pwolanin Exp $

/**
 * @file
 *   Administrative settings for searching.
 */

/**
 * Menu callback - the settings form.
 */
function apachesolr_search_settings_page() {
  $output = drupal_get_form('apachesolr_search_bias_form');
  // try to fetch the schema fields
  try {
    $solr = apachesolr_get_solr();
    $fields = $solr->getFields();
    $output .= drupal_get_form('apachesolr_search_settings_form', $fields);
  }
  catch (Exception $e) {
    watchdog('apachesolr', $e->getMessage());
    drupal_set_message($e->getMessage(), "warning");
    $output .= t('Cannot get information about the fields in the index at this time.');
  }

  return $output;
}

/**
 * Form builder function to set date bias.
 */
function apachesolr_search_bias_form($form_state) {

  $date_settings = variable_get('apacehsolr_search_date_boost', '4:3.0');
  $comment_settings = variable_get('apacehsolr_search_comment_boost', '4:3.0');
  
  $options = array(
    '0:0' => t('Omit'),
    '1:1.0' => '1',
    '2:1.0' => '2',
    '2:2.0' => '3',
    '4:2.0' => '4',
    '4:3.0' => '5',
    '4:5.0' => '6',
    '8:8.0' => '7',
    '8:13.0' => '8',
    '8:34.0' => '9',
    '10:100.0' => '10'
  );

  $form['biasing'] = array(
    '#type' => 'fieldset',
    '#title' => t('Result biasing'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Specify here biasing for different node properties when ordering the search results. Choose <em>Omit</em> to ignore this property.'),
  );
  $form['biasing']['apacehsolr_search_date_boost'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t("More recent change bias"),
    '#default_value' => $date_settings,
    '#description' => t('This setting will change the scoring so that more recent results may appear before those with higher keyword matching.'),
  );
  $form['biasing']['apacehsolr_search_comment_boost'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t("More comments bias"),
    '#default_value' => $comment_settings,
    '#description' => t('This setting will change the scoring so that nodes with more comments may appear before those with higher keyword matching.'),
  );

  return system_settings_form($form);
}

/**
 * Form builder function to set query field weights.
 */
function apachesolr_search_settings_form($form_state, $fields) {
  $form = array();

  // get the current weights
  $qf = variable_get('apachesolr_search_query_fields', array());
  $weights = array();
  $weights['0'] = t('Omit');
  $weights += drupal_map_assoc(array('0.1', '0.2', '0.3', '0.5', '0.8', '1.0', '2.0', '3.0', '5.0', '8.0', '13.0', '21.0'));
  // Note - we have default values set in solrconfig.xml, which will operate when 
  // none are set.
  $defaults = array(
    'body' => '1.0',
    'title' => '5.0',
    'name' => '3.0',
    'taxonomy_names' => '2.0',
    'tags_h1' => '5.0',
    'tags_h2_h3' => '3.0',
    'tags_h4_h5_h6' => '2.0',
    'tags_inline' => '1.0',
    'tags_a' => '0',
  );
  if (!$qf) {
    $qf = $defaults;
  }
  if ($fields) {

    $form['apachesolr_search_query_fields'] = array(
      '#type' => 'fieldset',
      '#title' => t('Field weights'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#tree' => TRUE,
      '#description' => t('Specify here which fields are more important when searching. Give a field a bigger numeric value to make it more important.  If you omit a field, it will not be searched for keywords.'),
    );
    foreach ($fields as $field_name => $field) {
      $form['apachesolr_search_query_fields'][$field_name] = array(
        '#access' => $field->type == 'text',
        '#type' => 'select',
        '#options' => $weights,
        '#title' => t('Weight for %field_name', array('%field_name' => $field_name)),
        '#default_value' => isset($qf[$field_name]) ? $qf[$field_name] : '0',
      );
    }
    // Make sure all the default fields are included, even if they have no indexed content.
    foreach ($defaults as $field_name => $weight) {
      $form['apachesolr_search_query_fields'][$field_name] = array(
        '#type' => 'select',
        '#options' => $weights,
        '#title' => t('Weight for %field_name', array('%field_name' => $field_name)),
        '#default_value' => isset($qf[$field_name]) ? $qf[$field_name] : $defaults[$field_name],
      );
    }

    ksort($form['apachesolr_search_query_fields']);
  }

  return system_settings_form($form);
}

