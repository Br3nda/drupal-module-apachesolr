<?php
// $Id: apachesolr.taxonomy.inc,v 1.5 2010/05/17 00:32:26 pwolanin Exp $

/**
 * Overrides taxonomy/term/X links
 */
//function apachesolr_search_taxonomy_term_page($str_tids = '', $depth = 0, $op = 'page') {
function apachesolr_search_taxonomy_term_page($term) {	
  // Build breadcrumb based on the hierarchy of the term.
  $current = (object) array(
    'tid' => $term->tid,
  );
  $breadcrumb = array();
  while ($parents = taxonomy_get_parents($current->tid)) {
    $current = array_shift($parents);
    $breadcrumb[] = l($current->name, 'taxonomy/term/' . $current->tid);
  }
  $breadcrumb[] = l(t('Home'), NULL);
  $breadcrumb = array_reverse($breadcrumb);
  drupal_set_breadcrumb($breadcrumb);
  drupal_add_feed(url('taxonomy/term/' . $term->tid . '/feed'), 'RSS - ' . $term->name);

  $build['term_heading'] = array(
    '#prefix' => '<div class="term-listing-heading">',
    '#suffix' => '</div>',
    'term' => taxonomy_term_view($term, 'full'),
  );

  $filters = 'tid:' . $term->tid;
  $solrsort = variable_get('apachesolr_search_taxonomy_sort', 'created desc');
  $page = isset($_GET['page']) ? $_GET['page'] : 0;
  $type = 'apachesolr_Search';
  $_GET['retain-filters'] = 1; //?? 

  $search_result = apachesolr_search_run('', $filters, $solrsort, 'search/apachesolr_search', $page);
    
  if ($search_result){
    $build['content'] = array(
      '#markup' => apachesolr_search_theme_search_results( $search_result, $type )
    );      	
  }
  else{
    $build['no_content'] = array(
      '#prefix' => '<p>',
      '#markup' => t('There is currently no content classified with this term.'),
      '#suffix' => '</p>',
    );  	
  }
  return $build;
}
