<?php

// $Id: apachesolr_views_integration.views.inc,v 1.2 2008/07/08 14:33:25 drunkenmonkey Exp $

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
    'argument' => array(
      'field' => 'nid',
      'handler' => 'views_handler_argument_apachesolr_query',
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
 * Class for filtering the node according to the apachesolr query. 
 */
class views_handler_argument_apachesolr_query extends views_handler_argument {

  /**
   * Display the filter on the administrative summary
   */
  function admin_summary() {
    return 'Display Apache Solr search results';
  }

  /**
   * Add a where clause to the query that leaves only the results of the
   * last apachesolr search in the view.
   * If no search was performed, don't show any nodes.
   */
  function query() {
    $this->ensure_my_table();

    // This is the object that does the communication with the solr server.
    $solr =& apachesolr_get_solr(variable_get('apachesolr_host', 'localhost'), variable_get('apachesolr_port', 8983), variable_get('apachesolr_path', '/solr'));
    // This is the object that knows about the query coming from the user.
    $query =& apachesolr_drupal_query($this->argument);
    
    try {
      $facet_limits = variable_get('apachesolr_facet_limits', array());
      $facet_limit = (count($facet_limits) > 0) ? max($facet_limits) : 10;
      $params = array(
        //'qt' => 'standard',
        'fl' => '*,score',
        'rows' => 1E6,
        'facet' => 'true',
        'facet.limit' => $facet_limit,
        'facet.field' => array('tid', 'type', 'uid', 'language'),
        'facet.mincount' => 1,
        'facet.sort' => 'true'
      );
      if ($fields = apachesolr_cck_fields()) {
        foreach ($fields as $name => $field) {
          $index_key = apachesolr_index_key($field);
          $params['facet.field'][] = $index_key;
        }
      }
      $response = $solr->search($query->get_query(), 0, 1E6, $params);
      apachesolr_has_searched(TRUE);
      apachesolr_static_response_cache($response);

      $nid_list = '';
      foreach ($response->documents as $doc) {
        $nid_list .= ', ' . $doc->nid;
      }

      if (strlen($nid_list) > 2) {
        $nid_list = substr($nid_list, 2);
        $where = "$this->table_alias.$this->real_field IN ($nid_list)";
      }
      else {
        $where = "0 = 1";
      }

      $this->query->add_where($this->options['group'], $where);
    }
    catch (Exception $e) {
      watchdog('Apache Solr', $e->getMessage(), WATCHDOG_ERROR);
    }
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
  $view->description = 'Display the results of the current Apache Solr search.';
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




  $view = new view;
  $view->name = 'apachesolr_search';
  $view->description = 'Search the site with Apache Solr';
  $view->tag = 'apachesolr';
  $view->view_php = '';
  $view->base_table = 'node';
  $view->is_cacheable = '0';
  $view->api_version = 2;
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->override_option('fields', array(
    'title' => array(
      'label' => 'Title',
      'link_to_node' => 1,
      'exclude' => 0,
      'id' => 'title',
      'table' => 'node',
      'field' => 'title',
      'relationship' => 'none',
    ),
    'timestamp' => array(
      'label' => 'Has new content',
      'link_to_node' => 0,
      'comments' => 1,
      'exclude' => 0,
      'id' => 'timestamp',
      'table' => 'history_user',
      'field' => 'timestamp',
      'relationship' => 'none',
    ),
    'name' => array(
      'label' => 'Name',
      'link_to_user' => 1,
      'exclude' => 0,
      'id' => 'name',
      'table' => 'users',
      'field' => 'name',
      'relationship' => 'none',
    ),
    'created' => array(
      'label' => 'Post date',
      'date_format' => 'large',
      'custom_date_format' => '',
      'exclude' => 0,
      'id' => 'created',
      'table' => 'node',
      'field' => 'created',
      'relationship' => 'none',
    ),
    'edit_node' => array(
      'label' => 'Edit link',
      'text' => '',
      'exclude' => 0,
      'id' => 'edit_node',
      'table' => 'node',
      'field' => 'edit_node',
      'relationship' => 'none',
    ),
    'delete_node' => array(
      'label' => 'Delete link',
      'text' => '',
      'exclude' => 0,
      'id' => 'delete_node',
      'table' => 'node',
      'field' => 'delete_node',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('arguments', array(
    'apachesolr' => array(
      'default_action' => 'ignore',
      'style_plugin' => 'default_summary',
      'style_options' => array(),
      'wildcard' => '*:*',
      'wildcard_substitution' => 'All',
      'title' => 'Search for %1',
      'default_argument_type' => 'fixed',
      'default_argument' => '',
      'validate_type' => 'none',
      'validate_fail' => 'not found',
      'id' => 'apachesolr',
      'table' => 'node',
      'field' => 'apachesolr',
      'relationship' => 'none',
      'default_argument_user' => 0,
      'default_argument_fixed' => '',
      'default_argument_php' => '',
      'validate_argument_node_type' => array(
        'blog' => 0,
        'page' => 0,
        'story' => 0,
      ),
      'validate_argument_node_access' => 0,
      'validate_argument_nid_type' => 'nid',
      'validate_argument_vocabulary' => array(
        '2' => 0,
      ),
      'validate_argument_type' => 'tid',
      'validate_argument_php' => '',
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
    'sticky' => 1,
    'order' => 'desc',
    'columns' => array(
      'delete_node' => 'edit_node',
      'edit_node' => 'edit_node',
      'timestamp' => 'title',
      'created' => 'created',
      'title' => 'title',
      'name' => 'name',
    ),
    'info' => array(
      'delete_node' => array(
        'separator' => '',
      ),
      'edit_node' => array(
        'separator' => ' | ',
      ),
      'timestamp' => array(
        'separator' => '',
      ),
      'created' => array(
        'sortable' => 1,
        'separator' => '',
      ),
      'title' => array(
        'sortable' => 1,
        'separator' => ' ',
      ),
      'name' => array(
        'sortable' => 1,
        'separator' => '',
      ),
    ),
    'default' => 'created',
  ));
  
  $views[$view->name] = $view;





  return $views;
}
