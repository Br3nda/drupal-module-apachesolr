<?php
// $Id: Solr_Base_Query.php,v 1.1.4.5 2008/10/27 11:04:56 jacobsingh Exp $

class Solr_Base_Query {

  /**
 * Luke query to find out what fields the Lucene index already knows about.
 * TODO: Does this belong in this class? Or in a Service class? Or in the module itself?
 */
  static function get_fields_in_index() {
    static $fields;
    if (empty($fields)) {
    // TODO: The apachesolr_base_url() is the only dependency on the module. Make it a static
    // class method?
      $response = drupal_http_request(apachesolr_base_url() ."/admin/luke?numTerms=0&wt=json");
      if ($response->code == '200') {
        $data = json_decode($response->data);
      }
    }
    return $data->fields;
  }

    /**
   * This is copied from search module. The search module implementation doesn't
   * handle quoted terms correctly (bug) and this function is copied here until
   * I have the bugfix perfected, at which point a patch will be submitted to search
   * module with the goal of removing the function here.
   *
   * Extract a module-specific search option from a search query. e.g. 'type:book'
   */
  static function query_extract($keys, $option) {
    $pattern = '/(^| )'. $option .':(\"([^\"]*)\")/i';
    preg_match_all($pattern, $keys, $matches);
    if (!empty($matches[2])) {
      // The preg_replace removes beginning and trailing quotations.
      return preg_replace('/^"|"$/', '', $matches[2]);
    }
    $pattern = '/(^| )'. $option .':([^ ]*)/i';
    if (preg_match_all($pattern, $keys, $matches)) {
      if (!empty($matches[2])) {
        return $matches[2];
      }
    }
  }

  /**
   * Replaces all occurances of $option in $keys.
   */
  static function query_replace($keys, $option) {
    $matches = Solr_Base_Query::query_extract($keys, $option);
    if (count($matches) > 0) {
      foreach ($matches as $match) {
        // TODO: Make some sort of name->value container object.
        $found = Solr_Base_Query::make_field(array('#name' => $option, '#value' => $match));
        $keys = str_replace($found, '', $keys);
      }
    }
    return $keys;
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
      // if the field value has spaces in it, wrap it in double quotes.
      if (count(explode(' ', $values['#value'])) > 1) {
        $values['#value'] = '"'. $values['#value']. '"';
      }
      return $values['#name']. ':'. $values['#value'];
    }
  }

  /**
   * A keyed array where the key is a position integer and the value
   * is an array with #name and #value properties.
   */
  private $_fields;

  /**
   * An array of subqueries.
   */
  private $_subqueries = array();

  /**
   * The query string.
   */
  private $_query;

  /**
   * Should fields be AND'd or OR'd together?
   */
  private $_field_operator;
  
  function __construct($query, $field_operator = "AND") {
    $this->_field_operator = $field_operator;
    $this->_query = trim($query);
    $this->parse_query();
  }

  function add_field($field, $value) {
    // microtime guarantees that added fields come at the end of the query,
    // in order.
    $this->_fields[microtime()] = array('#name' => $field, '#value' => trim($value));
    $this->rebuild_query();
  }
  
  function get_fields() {
    return $this->_fields;
  }

  function remove_field($name, $value = NULL) {
    // We can only remove named fields.
    if (empty($name)) {
      return;
    }
    if (empty($value)) {
      foreach ($this->_fields as $pos => $values) {
        if ($values['#name'] == $name) {
          unset($this->_fields[$pos]);
        }
      }
    }
    else {
      foreach ($this->_fields as $pos => $values) {
        if ($values['#name'] == $name && $values['#value'] == $value) {
          unset($this->_fields[$pos]);
        }
      }
    }
    $this->rebuild_query();
  }

  function has_field($name, $value) {
    foreach ($this->_fields as $pos => $values) {
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
  function add_subquery(Solr_Base_Query $query, $operator = 'AND') {
    $this->_subqueries[md5(serialize($query))] = array('#query' => $query, '#operator' => $operator);
  }
  
  function remove_subquery(Solr_Base_Query $query) {
    unset($this->_subqueries[md5(serialize($query))]);
  }
  
  function remove_subqueries() {
    $this->_subqueries = array();
  }
  
  function get_query() {
    $this->rebuild_query();
    return $this->_query;
  }

  /**
   * A function to get just the keyword components of the query,
   * omitting any field:value portions.
   */
  function get_query_basic() {
    $nonames = array_filter($this->_fields, create_function('$a', 'return empty($a[\'#name\']);'));
    $result = array();
    foreach ($nonames as $pos => $field) {
      $result[] = $field['#value'];
    }
    return implode(' ', $result);
  }

  function get_breadcrumb() {
    // This encodes an assumption that the breadcrumb is always building off
    // of the current page. Could be a problem.
    $breadcrumb = menu_get_active_breadcrumb();

    // double check that the fields are ordered by position.
    ksort($this->_fields);

    $progressive_crumb = array();
    // TODO: Don't know if hardcoding this is going to come back to bite.
    $base = 'search/'. arg(1). '/';

    foreach ($this->_fields as $field) {
      $progressive_crumb[] = Solr_Base_Query::make_field($field);
      if (empty($field['#name'])) {
        $breadcrumb[] = l($field['#value'], $base. implode(' ', $progressive_crumb));
      }
      else if ($themed = theme("apachesolr_breadcrumb_{$field['#name']}", $field['#value'])) {
        $breadcrumb[] = l($themed, $base. implode(' ', $progressive_crumb));
      }
      else {
        $breadcrumb[] = l($field['#value'], $base. implode(' ', $progressive_crumb));
      }
    }
    // the last breadcrumb is the current page, so it shouldn't be a link.
    $last = count($breadcrumb) - 1;
    $breadcrumb[$last] = strip_tags($breadcrumb[$last]);
    drupal_set_breadcrumb($breadcrumb);
    return $breadcrumb;
  }

  private function parse_query() {
    $this->_fields = array();
    $_keys = $this->_query;

    // Gets information about the fields already in solr index.
    $index_fields = Solr_Base_Query::get_fields_in_index();

    $rows = array();
    foreach ((array) $index_fields as $name => $field) {
      do {
        // save the strlen so we can detect if it has changed at the bottom
        // of the do loop
        $a = (int)strlen($_keys);
        // Get the values for $name
        $values = Solr_Base_Query::query_extract($_keys, $name);
        if (count($values) > 0) {
          foreach ($values as $value) {
            $found = Solr_Base_Query::make_field(array('#name' => $name, '#value' => $value));
            $pos = strpos($_keys, $found);
            // $solr_keys and $solr_crumbs are keyed on $pos so that query order
            // is maintained. This is important for breadcrumbs.
            $this->_fields[$pos] = array('#name' => $name, '#value' => trim($value));
          }
          // Update the local copy of $_keys by removing the key that was just found.
          $_keys = trim(Solr_Base_Query::query_replace($_keys, $name));
        }
        // Take new strlen to compare with $a.
        $b = (int)strlen($_keys);
      } while ($a !== $b);

      // Clean up by adding remaining keywords.
      if (!empty($_keys)) {
        $pos = strpos($this->_query, $_keys);
        $this->_fields[$pos] = array('#name' => '', '#value' => trim($_keys));
      }
    }
    // Even though the array has the right keys they are likely in the wrong
    // order. ksort() sorts the array by key while maintaining the key.
    ksort($this->_fields);
  }

  private function rebuild_query() {
    $fields = array();
    foreach ($this->_fields as $pos => $values) {
      $fields[] = Solr_Base_Query::make_field($values);
    }
    $join_delim = $this->_field_operator == 'AND' ? ' ' : ' OR ';    
    $this->_query = trim(implode($join_delim, array_filter($fields, 'trim')));
    foreach ($this->_subqueries as $id => $data) {
      $operator = $data['#operator'];
      $subquery = $data['#query']->get_query();
      $this->_query .= " {$operator} ({$subquery})";
    }
  }
}
