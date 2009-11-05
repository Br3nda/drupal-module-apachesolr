<?php
// $Id: apachesolr.d6.inc,v 1.1.2.2 2009/11/05 17:17:21 claudiucristea Exp $

/**
 * @file
 *   Functions from Drupal 6.
 *
 *   Make our life easily on backporting.
 */

function drupal_alter($type, &$data) {
  // PHP's func_get_args() always returns copies of params, not references, so
  // drupal_alter() can only manipulate data that comes in via the required first
  // param. For the edge case functions that must pass in an arbitrary number of
  // alterable parameters (hook_form_alter() being the best example), an array of
  // those params can be placed in the __drupal_alter_by_ref key of the $data
  // array. This is somewhat ugly, but is an unavoidable consequence of a flexible
  // drupal_alter() function, and the limitations of func_get_args().
  // @todo: Remove this in Drupal 7.
  if (is_array($data) && isset($data['__drupal_alter_by_ref'])) {
    $by_ref_parameters = $data['__drupal_alter_by_ref'];
    unset($data['__drupal_alter_by_ref']);
  }

  // Hang onto a reference to the data array so that it isn't blown away later.
  // Also, merge in any parameters that need to be passed by reference.
  $args = array(&$data);
  if (isset($by_ref_parameters)) {
    $args = array_merge($args, $by_ref_parameters);
  }

  // Now, use func_get_args() to pull in any additional parameters passed into
  // the drupal_alter() call.
  $additional_args = func_get_args();
  array_shift($additional_args);
  array_shift($additional_args);
  $args = array_merge($args, $additional_args);

  foreach (module_implements($type .'_alter') as $module) {
    $function = $module .'_'. $type .'_alter';
    call_user_func_array($function, $args);
  }
}

function db_placeholders($arguments, $type = 'int') {
  $placeholder = db_type_placeholder($type);
  return implode(',', array_fill(0, count($arguments), $placeholder));
}

function db_type_placeholder($type) {
  switch ($type) {
    case 'varchar':
    case 'char':
    case 'text':
    case 'datetime':
      return "'%s'";

    case 'numeric':
      // Numeric values are arbitrary precision numbers.  Syntacically, numerics
      // should be specified directly in SQL. However, without single quotes
      // the %s placeholder does not protect against non-numeric characters such
      // as spaces which would expose us to SQL injection.
      return '%n';

    case 'serial':
    case 'int':
      return '%d';

    case 'float':
      return '%f';

    case 'blob':
      return '%b';
  }

  // There is no safe value to return here, so return something that
  // will cause the query to fail.
  return 'unsupported type '. $type .'for db_type_placeholder';
}
