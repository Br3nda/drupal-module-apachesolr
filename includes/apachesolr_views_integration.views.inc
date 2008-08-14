<?php

// $Id: apachesolr_views_integration.views.inc,v 1.14 2008/08/14 15:43:41 drunkenmonkey Exp $

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
      'handler' => 'views_handler_field_apachesolr',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_apachesolr',
    ),
  );
  $data['apachesolr']['title'] = array(
    'title' => t('Title'),
    'help' => t('The title of the node.'),
    'field' => array(
      'handler' => 'views_handler_field_apachesolr',
      'click sortable' => TRUE,
     ),
    'sort' => array(
      'handler' => 'views_handler_sort_apachesolr',
    ),
  );
  $data['apachesolr']['changed'] = array(
    'title' => t('Updated date'),
    'help' => t('The date the node was last updated.'),
    'field' => array(
      'handler' => 'views_handler_field_apachesolr_date',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_apachesolr',
    ),
  );
  $data['apachesolr']['type'] = array(
    'title' => t('Type'),
    'help' => t('The type of a node (for example, "blog entry", "forum post", "story", etc).'),
    'field' => array(
      'handler' => 'views_handler_field_apachesolr_type',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_apachesolr',
    ),
  );
  $data['apachesolr']['name'] = array(
    'title' => t('Author'),
    'help' => t("The node's author."),
    'field' => array(
      'handler' => 'views_handler_field_apachesolr_author',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_apachesolr',
    ),
  );
  $data['apachesolr']['body'] = array(
    'title' => t('Body'),
    'help' => t("The node's content."),
    'field' => array(
      'handler' => 'views_handler_field_apachesolr_body',
      'click sortable' => FALSE,
    ),
  );
  $data['apachesolr']['comment_count'] = array(
    'title' => t('Comment count'),
    'help' => t('The number of comments that were posted to the node.'),
    'field' => array(
      'handler' => 'views_handler_field_apachesolr',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_apachesolr',
    ),
  );
  if (module_exists('translation')) {
    $data['apachesolr']['language'] = array(
      'title' => t('Language'),
      'help' => t('The language the node is in.'),
      'field' => array(
        'handler' => 'views_handler_field_apachesolr',
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort_apachesolr',
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
 * Default class for a field of the apachesolr base table. Inherits from the
 * node default class and overrides methods where necessary.
 */
class views_handler_field_apachesolr extends views_handler_field_node {
  
  public function ensure_my_table() {}
  public function query() {}
  
  /**
   * Render the field value.
   * 
   * This is copied from views_handler_field_node, but without the indirection
   * via $field_alias, since in Apache Solr there are no field aliases.
   */
  public function render($values) {
    return $this->render_link(check_plain($values->{$this->real_field}), $values);
  }
  
  /**
   * Render whatever the data is as a link to the node, if the according option
   * is set.
   * 
   * This is copied from views_handler_field_node, but without the indirection
   * via the aliases table, since in Apache Solr there are no field aliases.
   */
  public function render_link($data, $values) {
    if (!empty($this->options['link_to_node'])) {
      return l($data, "node/" . $values->nid, array('html' => TRUE));
    }
    else {
      return $data;
    }
  }
  
  /**
   * Called when click-sorting.
   */
   public function click_sort($order) {
     $this->query->set_param('sort',
        $this->real_field . ' ' . strtolower($order));
   }
  
}

/**
 * Class for rendering the node author.
 */
class views_handler_field_apachesolr_author
    extends views_handler_field_apachesolr {
  
  function option_definition() {
    $options = parent::option_definition();
    if (isset($options['link_to_node'])) {
      unset($options['link_to_node']);
    }
    $options['link_to_profile'] = array('default' => TRUE);
    return $options;
  }

  /**
   * Provide link_to_profile option
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    if (isset($form['link_to_node'])) {
      unset($form['link_to_node']);
    }
    $form['link_to_profile'] = array(
      '#title' => t("Link the name to the user's profile."),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_to_profile']),
    );
  }

  /**
   * Render the field. If the link_to_profile option is set, the name links to
   * the user's profile.
   */
  function render($values) {
    if (empty($this->options['link_to_profile'])) {
      return check_plain($values->name);
    }
    else {
      return l(check_plain($values->name),
          "user/" . $values->uid, array('html' => TRUE));
    }
  }
  
}

/**
 * Class for rendering the node body.
 */
class views_handler_field_apachesolr_body
    extends views_handler_field_apachesolr {
  
  function option_definition() {
    $options = parent::option_definition();
    if (isset($options['link_to_node'])) {
      unset($options['link_to_node']);
    }
    $options['teaser'] = array('default' => TRUE);
    $options['teaser_length'] =
        array('default' => variable_get('teaser_length', 600));
    return $options;
  }

  /**
   * Provide teaser option
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    if (isset($form['link_to_node'])) {
      unset($form['link_to_node']);
    }
    $form['teaser'] = array(
      '#title' => t('Show only teaser.'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['teaser']),
    );
    $form['teaser_length'] = array(
      '#title' => t('Teaser length'),
      '#type' => 'select',
      '#options' => array(
        100 => 100,
        200 => 200,
        300 => 300,
        400 => 400,
        500 => 500,
        600 => 600,
        700 => 700,
        800 => 800,
        900 => 900,
        1000 => 1000,
      ),
      '#default_value' => isset($this->options['teaser'])
                          ? $this->options['teaser']
                          : variable_get('teaser_length', 600),
      '#description' => t('The approximate length of the teaser, ' .
          'if displayed, in characters.'),
    );
  }

  /**
   * Render the field. If the teaser option is set, the name links to
   * the user's profile.
   */
   //TODO This should use check_plain() if the format doesn't allow HTML.
   //     Also, contained PHP code (if allowed by the format) is not handled.
  function render($values) {
    if (empty($this->options['teaser'])) {
      return $values->body;
    }
    else {
      return node_teaser($values->body, NULL, $this->options['teaser_length']);
    }
  }
  
}

/**
 * Class for rendering the node type.
 */
class views_handler_field_apachesolr_type
    extends views_handler_field_apachesolr {
  
  /**
   * Render the field value.
   */
  public function render($values) {
    return $this->render_link(node_get_types('name', $values->{$this->real_field}), $values);
  }
  
}

/**
 * Class for rendering the node type.
 */
class views_handler_field_apachesolr_date
    extends views_handler_field_apachesolr {
  
  function option_definition() {
    $options = parent::option_definition();
    $options['date_format'] =
        array('default' => 'medium');
    return $options;
  }

  /**
   * Provide teaser option
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['date_format'] = array(
      '#title' => t('Date format'),
      '#type' => 'select',
      '#options' => array(
        'small' => t('Small'),
        'medium' => t('Medium'),
        'large' => t('Large'),
      ),
      '#default_value' => $this->options['date_format'],
    );
  }
  
  /**
   * Render the field value.
   */
  public function render($values) {
    return $this->render_link(format_date($values->{$this->real_field},
        $this->options['date_format']), $values);
  }
  
}

/**
 * Class for sorting for a field.
 */
class views_handler_sort_apachesolr extends views_handler_sort {
  
  /**
   * Places the sort into the search parameters.
   */
  public function query() {
    $this->query->set_param('sort',
        $this->real_field . ' ' . strtolower($this->options['order']));
  }
  
}

/**
 * Class that allows searching the site with Apache Solr through a view.
 */
class apachesolr_search_views_argument_handler extends views_handler_argument {
  //TODO What needs to be added here?
  
  public function query() {
    $this->query->add_term($this->argument);
  }
  
  public function set_breadcrumb() {
    drupal_set_breadcrumb($this->query->get_breadcrumb());
  }
  
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
  
  /** The breadcrumb to be set if it is fit for the display. */
  private $_breadcrumb;
  
  
  /** Constructs this objects and fills it with default values. */
  public function __construct() {
    $this->_keys = array();
    $this->_params = apachesolr_search_get_params();
    _apachesolr_include_solrclient();
  }
  
  /** Adds the specified term to the search. */
  public function add_term($term, $field = 'text') {
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
          $keys[] = $term['term'];
        }
        else {
          $keys[] = $term['field'] . ':' . $term['term'];
        }
      }
      else {
        $keys[] = $term;
      }
    }
    $this->_keys[] = '(' . implode(' OR ', $keys) . ')';
  }
  
  /** Sets the specified Solr search parameter to the specified value. */
  public function set_param($param, $value) {
    $this->_params[$param] = $value;
  }
  
  /** Get the specified Solr search parameter. */
  public function get_param($param) {
    return isset($this->_params[$param]) ? $this->_params[$param] : '';
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
if (!is_array($doc->tid)) {//TODO
  file_put_contents('temp.txt', print_r($doc->tid, TRUE));
}
else
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
      $this->_breadcrumb = $query->get_breadcrumb();
      
      $this->_results = $ret;
      return $ret;
    }
    catch (Exception $e) {
      watchdog('Apache Solr', $e->getMessage(), NULL, WATCHDOG_ERROR);
    }
  }
  
  /** Return the breadcrumb. */
  public function get_breadcrumb() {
    return $this->_breadcrumb;
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
  static public function escape_term($term) {
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
