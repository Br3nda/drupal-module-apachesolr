<?php

// $Id: apachesolr_views_integration.views.inc,v 1.12 2008/08/11 13:42:47 drunkenmonkey Exp $

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
  
  
  $data['apachesolr']['table']['group']  = t('Apache Solr');

  $data['apachesolr']['table']['base'] = array(
    'query class' => 'apachesolr_views_query',
    'field' => 'nid',
    'title' => t('Apache Solr'),
    'help' => t('Searches the site with the Apache Solr search engine.'),
    'weight' => 0,
  );

  $data['apachesolr']['nid'] = array(
    'title' => t('Nid'),
    'help' => t('The node ID of the node.'),
    'field' => array(
      'handler' => 'apachesolr_dummy_views_field_handler',
      'click sortable' => FALSE,
    ),
  );
  $data['apachesolr']['title'] = array(
    'title' => t('Title'),
    'help' => t('The title of the node.'),
    'field' => array(
      'handler' => 'apachesolr_dummy_views_field_handler',
      'click sortable' => FALSE,
     ),
  );
  $data['apachesolr']['changed'] = array(
    'title' => t('Updated date'),
    'help' => t('The date the node was last updated.'),
    'field' => array(
      'handler' => 'apachesolr_dummy_views_field_handler',
      'click sortable' => FALSE,
    ),
  );
  $data['apachesolr']['type'] = array(
    'title' => t('Type'),
    'help' => t('The type of a node (for example, "blog entry", "forum post", "story", etc).'),
    'field' => array(
      'handler' => 'apachesolr_dummy_views_field_handler',
      'click sortable' => FALSE,
    ),
  );
  $data['apachesolr']['uid'] = array(
    'title' => t('Author Uid'),
    'help' => t("The node's author's user ID."),
    'field' => array(
      'handler' => 'apachesolr_dummy_views_field_handler',
      'click sortable' => FALSE,
    ),
  );
  $data['apachesolr']['name'] = array(
    'title' => t('Author name'),
    'help' => t("The node's author's name."),
    'field' => array(
      'handler' => 'apachesolr_dummy_views_field_handler',
      'click sortable' => FALSE,
    ),
  );
  $data['apachesolr']['body'] = array(
    'title' => t('Body'),
    'help' => t("The node's content."),
    'field' => array(
      'handler' => 'apachesolr_dummy_views_field_handler',
      'click sortable' => FALSE,
    ),
  );
  $data['apachesolr']['comment_count'] = array(
    'title' => t('Comment count'),
    'help' => t('The number of comments that were posted to the node.'),
    'field' => array(
      'handler' => 'apachesolr_dummy_views_field_handler',
      'click sortable' => FALSE,
    ),
  );
  if (module_exists('translation')) {
    $data['apachesolr']['language'] = array(
      'title' => t('Language'),
      'help' => t('The language the node is in.'),
      'field' => array(
        'handler' => 'apachesolr_dummy_views_field_handler',
        'click sortable' => FALSE,
      ),
    );
  }
  
  $data['apachesolr']['search'] = array(
    'title' => t('Search'),
    'help' => t('Searches the content with Solr'),
    'argument' => array(
      'field' => 'nid',
      'handler' => 'apachesolr_search_views_argument_handler',
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
      $params = array(
        //'qt' => 'standard',
        'fl' => '*,score',
        'rows' => variable_get('apachesolr_rows', 10),
        'facet' => 'true',
        'facet.field' => array('tid', 'type', 'uid', 'language'),
        'facet.mincount' => 1,
        'facet.sort' => 'true'
      );
      
      // Facet limits
      $facet_query_limits = variable_get('apachesolr_facet_query_limits', array());
      foreach ($facet_query_limits as $fieldname => $limit) {
        $params[$fieldname . '.facet.limit'] = $limit;
      }
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
 * Class for telling views about the fields apachesolr provides.
 * Since a search result always contains all fields, no query manipulation
 * whatsoever is necessary.
 */
class apachesolr_dummy_views_field_handler extends views_handler_field_node {
  
  public function ensure_my_table() {}
  
  public function query() {
    $this->field_alias = $this->real_field;
  }
  
}

/**
 * Class that allows searching the site with Apache Solr through a view.
 */
class apachesolr_search_views_argument_handler extends views_handler_argument {
  //TODO Add query(), etc.
}

/**
 * Implementation of hook_views_default_views().
 * 
 * Returns a view for displaying apachesolr search results.
 */
function apachesolr_views_integration_views_default_views() {
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
      'wildcard' => '',
      'wildcard_substitution' => '',
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

/**
 * Class for handling a view that gets its data not from the database, but from
 * a Solr server.
 */
/*
 * TODO: derive this from an abstract views query class that defines the methods
 * needed for executing the view.
 * Methods used by filters/arguments/... shouldn't/can't be included in this
 * base class, since they are too specfic to one back-end.
 */
include_once(drupal_get_path('module', 'views') . '/includes/query.inc');
class apachesolr_views_query extends views_query {
  
  /** Array storing the keys that will be used for the search. */
  private $_keys;
  
  /** Array containing the parameters that will be handed to Solr. */
  private $_params;
  
  /** Stores the search's result, once available. */
  private $_results;
  
  
  /** Constructs this objects and fills it with default values. */
  public function __construct() {
    $this->_keys = array();
    $this->_params = apachesolr_search_get_params();
    _apachesolr_include_solrclient();
  }
  
  /** Adds the specified term to the search. */
  public function add_term($term, $field = 'text') {
    $term = self::escapeTerm($term);
    if ($field != 'text') {
      $term = $field . ':' . $term; 
    }
    $this->_keys[] = $term;
  }
  
  /**
   * Add a group of search terms to the query, connected by OR operators.
   * 
   * @param $terms an array where each entry must be either of the form
   *     array('term' => $term, 'field' => $field);
   * (the 'field' entry can be omitted, in which case 'text' is assumed), or
   *     $term
   */
  public function add_or_group($terms) {
    if (count($terms) == 0 || !is_array($terms)) {
      return;
    }
    $keys = array();
    foreach ($terms as $term) {
      if (is_array($term)) {
        if (empty($term['field']) || $term['field'] == 'text') {
          $keys[] = self::escapeTerm($term['term']);
        }
        else {
          $keys[] = $term['field'] . ':' . self::escapeTerm($term['term']);
        }
      }
      else {
        $keys[] = self::escapeTerm($term);
      }
    }
    $this->_keys[] = '(' . implode(' OR ', $keys) . ')';
  }
  
  /** Sets the specified Solr search parameter to the specified value. */
  public function set_param($param, $value) {
    $this->_params[$param] = $value;
  }
  
  /**
   * Adds the specified parameters to the Solr search parameters, overwriting
   * old values where necessary. Parameters must be specified as
   * $param => $value in the array.
   */
  public function set_params($params) {
    $this->_params = $params + $this->_params;
  }
  
  /** Runs the search and returns the result. */
  public function execute(&$view) {
    if (!empty($this->_results)) {
      return $this->_results;
    }
    
    try {
      $start = microtime();
      $solr =& apachesolr_get_solr(variable_get('apachesolr_host', 'localhost'), variable_get('apachesolr_port', 8983), variable_get('apachesolr_path', '/solr'));
      $query =& apachesolr_drupal_query($this->_query);
      $response = $solr->search($query->get_query(), $this->_params['start'], $this->_params['rows'], $this->_params);
      apachesolr_has_searched(TRUE);
      
      apachesolr_static_response_cache($response);
      $results = array();
      $ret['total_rows'] = $response->numFound;
      
      if ($ret['total_rows'] > 0) {
        $fields = _apachesolr_fields();
        $fields[] = 'score';
        $results = array();
        foreach ($response->documents as $doc) {
          $result = array();
          foreach ($fields as $field) {
            $result[$field] = $doc->$field;
          }
          if (!empty($doc->tid)) {
            $taxonomies = array();
            while (!empty($doc->tid) && !empty($doc->taxonomy_name)) {
              $tid = array_shift($doc->tid);
              $name = array_shift($doc->taxonomy_name);
              $taxonomies[] = l($name, 'taxonomy/term/' . $tid, array('absolute' => TRUE));
            }
            $result['taxonomies'] = implode(', ', $taxonomies);
          }
          else {
            $result['taxonomies'] = '';
          }
          
          $results[] = (object) $result;
        }
      }
      
      $ret['result'] = $results;
      $ret['execute_time'] = microtime() - $start;
      
      // Set breadcrumb
      drupal_set_breadcrumb($query->get_breadcrumb());
      
      $this->_results = $ret;
      return $ret;
    }
    catch (Exception $e) {
      watchdog('Apache Solr', $e->getMessage(), NULL, WATCHDOG_ERROR);
    }
  }
  
  /** Set the start index of the returned results. */
  public function set_start($start) {
    $this->set_param('start', $start);
  }
  
  /** Set the number of returned results. */
  public function set_count($count) {
    $this->set_param('rows', $count);
  }
  
  /** Prepares the query. */
  public function query() {
    $this->_query = trim(implode(' ', $this->_keys));
    if (empty($this->_query)) {
      $this->_query = '*:*';
    }
  }
  
  
  /* Implementation of necessary dummy functions. */
  public function get_where_args() {}
  
  
  /** Escapes a term for passing it to the query. */
  static private function escape_term($term) {
    $term = trim($term);
    if (empty($term)) {
      return '';
    }
    if (($term{0} == '"' && $term{strlen($term)-1} == '"')
        || $term{0} == '(' && $term{strlen($term)-1} == ')') {
      return $term;
    }
    if (strpos($term, ' ') !== FALSE) {
      return Apache_Solr_Service::phrase($term);
    }
    return Apache_Solr_Service::escape($term);
  }
  
}
