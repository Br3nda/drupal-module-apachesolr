<?php
// $Id: Solr_Base_Query.php,v 1.1.4.28 2009/04/16 15:26:44 pwolanin Exp $

class Solr_Base_Query implements Drupal_Solr_Query_Interface {

  /**
   * Extract all uses of one named field from a filter string e.g. 'type:book'
   */
  public function filter_extract(&$filterstring, $name) {
    $extracted = array();
    // Range queries.  The "TO" is case-sensitive.
    $patterns[] = '/(^| )'. $name .':([\[\{](\S+) TO (\S+)[\]\}])/';
    // Match quoted values.
    $patterns[] = '/(^| )'. $name .':"([^"]*)"/';
    // Match unquoted values.
    $patterns[] = '/(^| )'. $name .':([^ ]*)/';
    foreach ($patterns as $p) {
      if (preg_match_all($p, $filterstring, $matches, PREG_SET_ORDER)) {
        foreach($matches as $match) {
          $filter = array();
          $filter['#query'] = $match[0];
          $filter['#value'] = trim($match[2]);
          if (isset($match[3])) {
            // Extra data for range queries
            $filter['#start'] = $match[3];
            $filter['#end'] = $match[4];
          }
          $extracted[] = $filter;
          // Update the local copy of $filters by removing the match.
          $filterstring = str_replace($match[0], '', $filterstring);
        }
      }
    }
    return $extracted;
  }

  /**
   * Takes an array $field and combines the #name and #value in a way
   * suitable for use in a Solr query.
   */
  public function make_filter(array $filter) {
    // If the field value has spaces, or : in it, wrap it in double quotes.
    // unless it is a range query.
    if (preg_match('/[ :]/', $filter['#value']) && !isset($filter['#start']) && !preg_match('/[\[\{]\S+ TO \S+[\]\}]/', $filter['#value'])) {
      $filter['#value'] = '"'. $filter['#value']. '"';
    }
    return $filter['#name'] . ':' . $filter['#value'];
  }

  /**
   * Static shared by all instances, used to increment ID numbers.
   */
  protected static $idCount = 0;

  /**
   * Each query/subquery will have a unique ID
   */
  protected $id;

  /**
   * A keyed array where the key is a position integer and the value
   * is an array with #name and #value properties.  Each value is a
   * used for filter queries, e.g. array('#name' => 'uid', '#value' => 0)
   * for anonymous content.
   */
  protected $fields;

  /**
   * The complete filter string for a query.  Usually from $_GET['filters']
   * Contains name:value pairs for filter queries.  For example,
   * "type:book" for book nodes.
   */
  protected $filterstring;

  /**
   * A mapping of field names from the URL to real index field names.
   */
  protected $field_map = array();

  /**
   * An array of subqueries.
   */
  protected $subqueries = array();

  /**
   * The query path (search keywords).
   */
  protected $querypath;

  /**
   * Apache_Solr_Service object
   */
  protected $solr;

  protected $available_sorts;

  /**
   * @param $solr
   *  An instantiated Apache_Solr_Service Object.
   *  Can be instantiated from apachesolr_get_solr().
   *
   * @param $querypath
   *   The string that a user would type into the search box. Suitable input
   *   may come from search_get_keys()
   *
   * @param $filterstring
   *   Key and value pairs that are applied as a filter query.
   *
   * @param $sortstring
   *   Visible string telling solr how to sort - added to output querystring.
   */
  function __construct($solr, $querypath, $filterstring, $sortstring) {
    $this->solr = $solr;
    $this->querypath = trim($querypath);
    $this->filterstring = trim($filterstring);
    $this->solrsort = trim($sortstring);
    $this->id = ++self::$idCount;
    $this->parse_filters();
    $this->available_sorts = $this->default_sorts();
  }

  function __clone() {
    $this->id = ++self::$idCount;
  }

  public function add_filter($field, $value) {
    $this->fields[] = array('#name' => $field, '#value' => trim($value));
  }

  public function get_filters($name = NULL) {
    if (empty($name)) {
      return $this->fields;
    }
    reset($this->fields);
    $matches = array();
    foreach ($this->fields as $filter) {
      if ($filter['#name'] == $name) {
        $matches[] = $filter;
      }
    }
    return $matches;
  }

  public function remove_filter($name, $value = NULL) {
    // We can only remove named fields.
    if (empty($name)) {
      return;
    }
    if (!isset($value)) {
      foreach ($this->fields as $pos => $values) {
        if ($values['#name'] == $name) {
          unset($this->fields[$pos]);
        }
      }
    }
    else {
      foreach ($this->fields as $pos => $values) {
        if ($values['#name'] == $name && $values['#value'] == $value) {
          unset($this->fields[$pos]);
        }
      }
    }
  }

  public function has_filter($name, $value) {
    foreach ($this->fields as $pos => $values) {
      if (!empty($values['#name']) && !empty($values['#value']) && $values['#name'] == $name && $values['#value'] == $value) {
        return TRUE;
      }
    }
    return FALSE;
  }

  /**
   * Handle aliases for field to make nicer URLs
   *
   * @param $field_map
   *   An array keyed with real Solr index field names, with value being the alias.
   */
  function add_field_aliases($field_map) {
    $this->field_map = array_merge($this->field_map, $field_map);
    // We have to re-parse the filters.
    $this->parse_filters();
  }

  function get_field_aliases() {
    return $this->field_map;
  }

  function clear_field_aliases() {
    $this->field_map = array();
    // We have to re-parse the filters.
    $this->parse_filters();
  }

  /**
   * A subquery is another instance of a Solr_Base_Query that should be joined
   * to the query. The operator determines whether it will be joined with AND or
   * OR.
   *
   * @param $query
   *   An instance of Drupal_Solr_Query_Interface.
   *
   * @param $operator
   *   'AND' or 'OR'
   */
  public function add_subquery(Drupal_Solr_Query_Interface $query, $fq_operator = 'OR', $q_operator = 'AND') {
    $this->subqueries[$query->id] = array('#query' => $query, '#fq_operator' => $fq_operator, '#q_operator' => $q_operator);
  }

  public function remove_subquery(Solr_Base_Query $query) {
    unset($this->subqueries[$query->id]);
  }

  public function remove_subqueries() {
    $this->subqueries = array();
  }

  public function set_solrsort($sortstring) {
    $this->solrsort = trim($sortstring);
  }

  public function get_available_sorts() {
    return $this->available_sorts;
  }

  public function set_available_sort($field, $sort) {
    $this->available_sorts[$field] = $sort;
  }

  /**
   * Returns a default list of sorts.
   */
  protected function default_sorts() {
    return array(
      'relevancy' => array('name' => t('Relevancy'), 'default' => 'asc'),
      'sort_title' => array('name' => t('Title'), 'default' => 'asc'),
      'type' => array('name' => t('Type'), 'default' => 'asc'),
      'sort_name' => array('name' => t('Author'), 'default' => 'asc'),
      'created' => array('name' => t('Date'), 'default' => 'desc'),
    );
  }

  /**
   * Return filters and sort in a form suitable for a query param to url().
   */
  public function get_url_querystring() {
    $querystring = '';
    if ($fq = $this->rebuild_fq(TRUE)) {
      foreach ($fq as $key => $value) {
        $fq[$key] = drupal_urlencode($value);
      }
      $querystring = 'filters='. implode('+', $fq);
    }
    if ($this->solrsort) {
      $querystring .= ($querystring ? '&' : '') .'solrsort='. $this->solrsort;
    }
    return $querystring;
  }

  public function get_fq() {
    return $this->rebuild_fq();
  }

  /**
   * A function to get just the keyword components of the query,
   * omitting any field:value portions.
   */
  public function get_query_basic() {
    return $this->rebuild_query();
  }
  
  /**
   * return the search path
   * this class assumes its always through the search api
   *
   * @param string $new_keywords
   * if we are using new keywords as our query string
   */
  public function get_path($new_keywords = NULL) {
    if ($new_keywords) {
      return 'search/' . arg(1) . '/' . $new_keywords;
    }
    return 'search/' . arg(1) . '/' . $this->get_query_basic();
  }

  /**
   * Build additional breadcrumb elements relative to $base.
   */
  public function get_breadcrumb($base = '') {
    $progressive_crumb = array();

    $search_keys = $this->get_query_basic();
    if ($search_keys) {
      $breadcrumb[] = l($search_keys, $base);
    }

    foreach ($this->fields as $field) {
      $name = $field['#name'];
      $prefix = '';
      // Handle negative queries.
      if ($name[0] == '-') {
        $name = substr($name, 1);
        $prefix = '-';
      }
      // Look for a field alias.
      if (isset($this->field_map[$name])) {
        $field['#name'] = $prefix . $this->field_map[$name];
      }
      $progressive_crumb[] = $this->make_filter($field);
      $options = array('query' => 'filters=' . implode(' ', $progressive_crumb));
      if ($themed = theme("apachesolr_breadcrumb_{$name}", $field['#value'])) {
        $breadcrumb[] = l($themed, $base, $options);
      }
      else {
        $breadcrumb[] = l($field['#name'], $base, $options);
      }
    }
    // The last breadcrumb is the current page, so it shouldn't be a link.
    $last = count($breadcrumb) - 1;
    $breadcrumb[$last] = strip_tags($breadcrumb[$last]);

    return $breadcrumb;
  }

  /**
   * Parse the filter string in $this->filters into $this->fields.
   *
   * Builds an array of field name/value pairs.
   */
  protected function parse_filters() {
    $this->fields = array();
    $filterstring = $this->filterstring;

    // Gets information about the fields already in solr index.
    $index_fields = $this->solr->getFields();

    foreach ((array) $index_fields as $name => $data) {
      // Look for a field alias.
      $alias = isset($this->field_map[$name]) ? $this->field_map[$name] : $name;
      // Look for normal and negative queries for the same field.
      foreach(array('', '-') as $prefix) {
        // Get the values for $name
        $extracted = $this->filter_extract($filterstring, $prefix . $alias);
        if (count($extracted)) {
          foreach ($extracted as $filter) {
            $pos = strpos($this->filterstring, $filter['#query']);
            // $solr_keys and $solr_crumbs are keyed on $pos so that query order
            // is maintained. This is important for breadcrumbs.
            $filter['#name'] = $prefix . $name;
            $this->fields[$pos] = $filter;
          }
        }
      }
    }
    // Even though the array has the right keys they are likely in the wrong
    // order. ksort() sorts the array by key while maintaining the key.
    ksort($this->fields);
  }

  /**
   * Builds a set of filter queries from $this->fields and all subqueries.
   *
   * Returns an array of strings that can be combined into
   * a URL query parameter or passed to Solr as fq paramters.
   */
  protected function rebuild_fq($aliases = FALSE) {
    $fq = array();
    $fields = array();
    foreach ($this->fields as $pos => $field) {
      // Look for a field alias.
      if ($aliases && isset($this->field_map[$field['#name']])) {
        $field['#name'] = $this->field_map[$field['#name']];
      }
      $fq[] = $this->make_filter($field);
    }
    foreach ($this->subqueries as $id => $data) {
      $subfq = $data['#query']->rebuild_fq($aliases);
      if ($subfq) {
        $operator = $data['#fq_operator'];
        $fq[] = "(" . implode(" {$operator} ", $subfq) .")";
      }
    }
    return $fq;
  }

  protected function rebuild_query() {
    $query = $this->querypath;
    foreach ($this->subqueries as $id => $data) {
      $operator = $data['#q_operator'];
      $subquery = $data['#query']->get_query_basic();
      if ($subquery) {
        $query .= " {$operator} ({$subquery})";
      }
    }
    return $query;
  }
}
