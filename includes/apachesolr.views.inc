<?php

// $Id: apachesolr.views.inc,v 1.1 2008/06/30 17:46:45 drunkenmonkey Exp $

/**
 * Implementation of hook_views_data().
 */
function apachesolr_views_data() {
  
  $data['node']['apachesolr'] = array(
    'group' => t('Apachesolr'),
    'title' => t('Apache Solr search'),
    'help' => t('Only shows the nodes returned by the current Apache Solr search.'),
    'filter' => array(
      'field' => 'nid',
      'handler' => 'views_handler_filter_apachesolr_query',
    ),
  );
  
  return $data;
}

/**
 * Class for filtering the node according to the apachesolr query. 
 */
class views_handler_filter_apachesolr_query extends views_handler_filter {

  /**
   * Display the filter on the administrative summary
   */
  function admin_summary() {
    return 'Filter by Apache Solr search';
  }

  /**
   * Add a where clause to the query that leaves only the results of the
   * last apachesolr search in the view.
   * If no search was performed, don't show any nodes.
   */
  function query() {
    $this->ensure_my_table();
    
    if (!apachesolr_has_searched()) {
      $where = "0 = 1";
    }
    else {
      $response =& apachesolr_static_response_cache();
      $nids = '';
      foreach ($response->documents as $doc) {
        $nids .= ', ' . $doc->nid;
      }
      
      if (strlen($nids) > 2) {
        $nids = substr($nids, 2);
        $where = "$this->table_alias.$this->real_field IN ($nids)";
      }
      else {
        $where = "0 = 1";
      }
    }
    $this->query->add_where($this->options['group'], $where);
  }

}

/**
 * Implementation of hook_views_default_views().
 * 
 * Returns a view for displaying apachesolr search results.
 */
function apachesolr_views_default_views() {
  $view = new view;
  $view->name = 'apachesolr_search_result';
  $view->description = 'Display the result of the current Apache Solr search.';
  $view->tag = 'apachesolr';
  $view->view_php = '';
  $view->base_table = 'node';
  $view->is_cacheable = '0';
  $view->api_version = 2;
  $view->disabled = FALSE;
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->override_option('fields', array(
    'title' => array(
      'id' => 'title',
      'table' => 'node',
      'field' => 'title',
    ),
    'view_node' => array(
      'id' => 'view_node',
      'table' => 'node',
      'field' => 'view_node',
    ),
    'timestamp' => array(
      'id' => 'timestamp',
      'table' => 'history_user',
      'field' => 'timestamp',
    ),
    'name' => array(
      'id' => 'name',
      'table' => 'users',
      'field' => 'name',
    ),
    'created' => array(
      'id' => 'created',
      'table' => 'node',
      'field' => 'created',
    ),
    'edit_node' => array(
      'id' => 'edit_node',
      'table' => 'node',
      'field' => 'edit_node',
    ),
    'delete_node' => array(
      'id' => 'delete_node',
      'table' => 'node',
      'field' => 'delete_node',
    ),
  ));
  $handler->override_option('filters', array(
    'status_extra' => array(
      'operator' => '=',
      'value' => '',
      'group' => '0',
      'exposed' => FALSE,
      'expose' => array(
        'operator' => FALSE,
        'label' => '',
      ),
      'id' => 'status_extra',
      'table' => 'node',
      'field' => 'status_extra',
      'relationship' => 'none',
    ),
    'apachesolr' => array(
      'operator' => '=',
      'value' => '',
      'group' => '0',
      'exposed' => FALSE,
      'expose' => array(
        'operator' => FALSE,
        'label' => '',
      ),
      'id' => 'apachesolr',
      'table' => 'node',
      'field' => 'apachesolr',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('access', array(
    'type' => 'none',
    'role' => array(),
    'perm' => '',
  ));
  $handler->override_option('title', 'Search results');
  $handler->override_option('use_ajax', TRUE);
  $handler->override_option('use_pager', '1');
  $handler->override_option('style_plugin', 'table');
  $handler->override_option('style_options', array(
    'grouping' => '',
    'override' => 1,
    'sticky' => 0,
    'order' => 'asc',
    'columns' => array(
      'title' => 'title',
      'view_node' => 'title',
      'timestamp' => 'title',
      'name' => 'name',
      'created' => 'created',
      'edit_node' => 'edit_node',
      'delete_node' => 'edit_node',
    ),
    'info' => array(
      'title' => array(
        'sortable' => 1,
        'separator' => ' ',
      ),
      'view_node' => array(
        'separator' => '',
      ),
      'timestamp' => array(
        'separator' => '',
      ),
      'name' => array(
        'sortable' => 1,
        'separator' => '',
      ),
      'created' => array(
        'sortable' => 1,
        'separator' => '',
      ),
      'edit_node' => array(
        'separator' => ' | ',
      ),
      'delete_node' => array(
        'separator' => '',
      ),
    ),
    'default' => 'title',
  ));
  $handler = $view->new_display('block', 'Block', 'block_1');
  $handler->override_option('block_description', 'ApacheSolr: Search results');
  
  
  
  // Add view to list of views to provide.
  $views[$view->name] = $view;

  // ...Repeat all of the above for each view the module should provide.

  // At the end, return array of default views.
  return $views;
}
