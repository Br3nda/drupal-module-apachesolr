<?php
// $Id: Solr_Base_Query.php,v 1.1.4.18 2009/02/05 18:33:25 pwolanin Exp $

class Solr_Base_Query {

  /**
   * This is copied from search module. The search module implementation doesn't
   * handle quoted terms correctly (bug) and this function is copied here until
   * I have the bugfix perfected, at which point a patch will be submitted to search
   * module with the goal of removing the function here.
   *
   * Extract a module-specific search option from a search query. e.g. 'type:book'
   */
  static function query_extract($filters, $option) {
    $pattern = '/(^| )'. $option .':"([^"]*)"/i';
    if (preg_match_all($pattern, $filters, $matches)) {
      return array('matches' => $matches[0], 'values' => $matches[2]);
    }
    $pattern = '/(^| )'. $option .':([^ ]*)/i';
    if (preg_match_all($pattern, $filters, $matches)) {
      return array('matches' => $matches[0], 'values' => $matches[2]);
    }
  }

  /**
   * Takes an array $values and combines the #name and #value in a way
   * suitable for use in a Solr query.
   */
  static function make_field(array $values) {
    if (empty($values['#name'])) {
      return implode(' ', array_filter(explode(' ', $values['#value']), 'trim'));
    }
    else {
      // If the field value has spaces, or : in it, wrap it in double quotes.
      if (preg_match('/[ :]/', $values['#value'])) {
        $values['#value'] = '"'. $values['#value']. '"';
      }
      return $values['#name'] . ':' . $values['#value'];
    }
  }

  /**
   * Static shared by all instances, used to increment ID numbers.
   */
  protected static $idCount = 0;

  /**
   * Each query/subquery will have a unique ID
   */
  public $id;

  /**
   * A keyed array where the key is a position integer and the value
   * is an array with #name and #value properties.
   */
  protected $fields;
  protected $filters;

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
    $this->filters = trim($filterstring); 
    $this->solrsort = trim($sortstring);
    $this->id = ++self::$idCount;
    $this->parse_query();
  }

  function __clone() {
    $this->id = ++self::$idCount;
  }

  function add_field($field, $value) {
    // microtime guarantees that added fields come at the end of the query,
    // in order.
    $this->fields[microtime()] = array('#name' => $field, '#value' => trim($value));
  }

  public function get_fields() {
    return $this->fields;
  }

  public function remove_field($name, $value = NULL) {
    // We can only remove named fields.
    if (empty($name)) {
      return;
    }
    if (empty($value)) {
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

  public function has_field($name, $value) {
    foreach ($this->fields as $pos => $values) {
      if (!empty($values['#name']) && !empty($values['#value']) && $values['#name'] == $name && $values['#value'] == $value) {
        return TRUE;
      }
    }
    return FALSE;
  }

  /**
   * A subquery is another instance of a Solr_Base_Query that should be joined
   * to the query. The operator determines whether it will be joined with AND or
   * OR.
   *
   * @param $query
   *   An instance of Solr_Base_Query.
   *
   * @param $operator
   *   'AND' or 'OR'
   */
  function add_subquery(Solr_Base_Query $query, $fq_operator = 'OR', $q_operator = 'AND') {
    $this->subqueries[$query->id] = array('#query' => $query, '#fq_operator' => $fq_operator, '#q_operator' => $q_operator);
  }

  function remove_subquery(Solr_Base_Query $query) {
    unset($this->subqueries[$query->id]);
  }

  public function remove_subqueries() {
    $this->subqueries = array();
  }

  public function set_solrsort($sortstring) {
    $this->solrsort = trim($sortstring);
  }
  /**
   * Return filters and sort in a form suitable for a query param to url().
   */
  public function get_url_querystring() {
    $querystring = '';
    if ($fq = $this->get_fq()) {
      $querystring = 'filters='. implode(' ', $fq);
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

  public function get_breadcrumb() {
    // This encodes an assumption that the breadcrumb is always building off
    // of the current page. Could be a problem.
    $breadcrumb = menu_get_active_breadcrumb();

    // double check that the fields are ordered by position.
    ksort($this->fields);

    $progressive_crumb = array();
    $search_keys = $this->get_query_basic();
    // TODO: Don't know if hardcoding this is going to come back to bite.
    $base = 'search/'. arg(1) . '/' . $search_keys;
    if ($search_keys) {
      $breadcrumb[] = l($search_keys, $base);
    }

    foreach ($this->fields as $field) {
      $progressive_crumb[] = Solr_Base_Query::make_field($field);
      $options = array('query' => 'filters=' . implode(' ', $progressive_crumb));
      if (empty($field['#name'])) {
        $breadcrumb[] = l($field['#value'], $base, $options);
      }
      else if ($themed = theme("apachesolr_breadcrumb_{$field['#name']}", $field['#value'])) {
        $breadcrumb[] = l($themed, $base, $options);
      }
      else {
        $breadcrumb[] = l($field['#name'], $base, $options);
      }
    }
    // the last breadcrumb is the current page, so it shouldn't be a link.
    $last = count($breadcrumb) - 1;
    $breadcrumb[$last] = strip_tags($breadcrumb[$last]);
    drupal_set_breadcrumb($breadcrumb);
    return $breadcrumb;
  }

  protected function parse_query() {
    $this->fields = array();
    $filters = $this->filters;

    // Gets information about the fields already in solr index.
    $index_fields = $this->solr->getFields();

    $rows = array();
    foreach ((array) $index_fields as $name => $field) {
      do {
        // save the strlen so we can detect if it has changed at the bottom
        // of the do loop
        $a = (int)strlen($filters);
        // Get the values for $name
        $extracted = Solr_Base_Query::query_extract($filters, $name);
        if (count($extracted['values'])) {
          foreach ($extracted['values'] as $value) {
            $found = Solr_Base_Query::make_field(array('#name' => $name, '#value' => $value));
            $pos = strpos($this->filters, $found);
            // $solr_keys and $solr_crumbs are keyed on $pos so that query order
            // is maintained. This is important for breadcrumbs.
            $this->fields[$pos] = array('#name' => $name, '#value' => trim($value));
          }
          // Update the local copy of $filters by removing the key that was just found.
          $filters = trim(str_replace($extracted['matches'], '', $filters));
        }
        // Take new strlen to compare with $a.
        $b = (int)strlen($filters);
      } while ($a !== $b);
    }
    // Even though the array has the right keys they are likely in the wrong
    // order. ksort() sorts the array by key while maintaining the key.
    ksort($this->fields);
  }

  protected function rebuild_fq() {
    $fields = array();
    foreach ($this->fields as $pos => $values) {
      $fields[] = Solr_Base_Query::make_field($values);
    }
    $fq = array_filter($fields, 'trim');
    foreach ($this->subqueries as $id => $data) {
      $subfq = $data['#query']->get_fq();
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
