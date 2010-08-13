<?php
// $Id: apachesolr.index.inc,v 1.13 2010/08/13 21:16:22 pwolanin Exp $

/**
 * @file
 *   Functions used when indexing content to Apache Solr.
 */

/**
 * Strip html tags and also control characters that cause Jetty/Solr to fail.
 */
function apachesolr_clean_text($text) {
  // Add spaces before stripping tags to avoid running words together.
  $text = filter_xss(str_replace(array('<', '>'), array(' <', '> '), $text), array());
  // Decode entities and then make safe any < or > characters.
  return htmlspecialchars(html_entity_decode($text, ENT_NOQUOTES, 'UTF-8'), ENT_NOQUOTES, 'UTF-8');
}

/**
 * Given a node, return a document representing that node.
 */
function apachesolr_node_to_document($node, $namespace) {
  $document = FALSE;
  // Let any module exclude this node from the index.
  $build_document = TRUE;
  foreach (module_implements('apachesolr_node_exclude') as $module) {
    $exclude = module_invoke($module, 'apachesolr_node_exclude', $node, $namespace);
    if (!empty($exclude)) {
      $build_document = FALSE;
    }
  }

  if ($build_document) {
    // Build the node body.
    $build = node_view($node, 'search_index');
    // Why do we need this?
    unset($build['#theme']);
    $text = drupal_render($build);

    $node->title = apachesolr_clean_text($node->title);
    // Fetch extra data normally not visible, including comments.
    // We do this manually (with module_implements instead of node_invoke_nodeapi)
    // because we want a keyed array to come back. Only in this way can we decide
    // whether to index comments or not.
    $extra = array();
    $exclude_comments = in_array($node->type, variable_get('apachesolr_exclude_comments_types', array()), TRUE);
    foreach (module_implements('node_update_index') as $module) {
      if ($exclude_comments && $module == 'comment') {
        // Don't add comments.
        continue;
      }
      $function = $module . '_node_update_index';
      if ($output = $function($node)) {
        $extra[$module] = $output;
      }

    }
    if (!variable_get('apachesolr_index_comments_with_node', TRUE)) {
      unset($extra['comment']);
    }

    $text .= "\n\n" . implode(' ', $extra);

    $document = new Apache_Solr_Document();
    $document->id = apachesolr_document_id($node->nid);
    $document->site = url(NULL, array('absolute' => TRUE));
    $document->hash = apachesolr_site_hash();
    $document->entity = 'node';
    $document->nid = $node->nid;
    $document->uid = $node->uid;
    $document->title = $node->title;
    $document->status = $node->status;
    $document->sticky = $node->sticky;
    $document->promote = $node->promote;
    $document->tnid = $node->tnid;
    $document->translate = $node->translate;
    if (empty($node->language)) {
      // 'und' is the language-neutral code in Drupal 7.
      $document->language = LANGUAGE_NONE;
    }
    else {
      $document->language = $node->language;
    }
    $document->body = apachesolr_clean_text($text);
    if (isset($node->teaser)) {
      $document->teaser = apachesolr_clean_text($node->teaser);
    }
    else {
      $document->teaser = truncate_utf8($document->body, 300, TRUE);
    }
    $document->type = $node->type;
    $document->type_name = node_type_get_name($node);
    $document->created = apachesolr_date_iso($node->created);
    $document->changed = apachesolr_date_iso($node->changed);
    $last_change = (isset($node->last_comment_timestamp) && $node->last_comment_timestamp > $node->changed) ? $node->last_comment_timestamp : $node->changed;
    $document->last_comment_or_change = apachesolr_date_iso($last_change);
    $document->comment_count = isset($node->comment_count) ? $node->comment_count : 0;
    $document->name = $node->name;

    $path = 'node/' . $node->nid;
    $document->url = url($path, array('absolute' => TRUE));
    $document->path = $path;
    // Path aliases can have important information about the content.
    // Add them to the index as well.
    if (function_exists('drupal_get_path_alias')) {
      // Add any path alias to the index, looking first for language specific
      // aliases but using language neutral aliases otherwise.
      $language = empty($node->language) ? NULL : $node->language;
      $output = drupal_get_path_alias($path, $language);
      if ($output && $output != $path) {
        $document->path_alias = $output;
      }
    }

    // Handle fields including taxonomy.
    $indexed_fields = apachesolr_node_fields();
    foreach ($indexed_fields as $index_key => $field_info) {
      $field_name = $field_info['field']['field_name'];
      // See if the node has fields that can be indexed
      if (isset($node->{$field_name})) {
        // Got a field.
        $function = $field_info['indexing_callback'];
        if ($function && function_exists($function)) {
          // NOTE: This function should always return an array.  One
          // node field may be indexed to multiple Solr fields.
          $fields = $function($node, $field_name, $index_key, $field_info);
          foreach ($fields as $field) {
            // It's fine to use this method also for single value fields.
            $document->setMultiValue($field['key'], $field['value']);
          }
        }
      }
    }

    // Index book module data.
    if (!empty($node->book['bid'])) {
      // Hard-coded - must change if apachesolr_index_key() changes.
      $document->is_book_bid = (int) $node->book['bid'];
    }
    apachesolr_add_tags_to_document($document, $text);

    // Let modules add to the document.
    foreach (module_implements('apachesolr_update_index') as $module) {
      $function = $module .'_apachesolr_update_index';
      $function($document, $node, $namespace);
    }
  }
  return $document;
}

/**
 * Callback that converts term_reference field into an array
 */
function apachesolr_term_reference_indexing_callback($node, $field_name, $index_key, $field_info) {
  $fields = array();
  $vocab_names = array();
  if (!empty($node->{$field_name}) && function_exists('taxonomy_get_parents_all')) {
    $field = $node->$field_name;
    list($lang, $items) = each($field);
      foreach ($items as $item) {
        // Double indexing of tids lets us do effecient searches (on tid)
        // and do accurate per-vocabulary faceting.

        // By including the ancestors to a term in the index we make
        // sure that searches for general categories match specific
        // categories, e.g. Fruit -> apple, a search for fruit will find
        // content categorized with apple.
        $ancestors = taxonomy_get_parents_all($item['tid']);
        foreach ($ancestors as $ancestor) {
          $fields[] = array(
            'key' => 'tid',
            'value' => $ancestor->tid,
          );
          $fields[] = array(
            'key' => 'im_vid_'. $ancestor->vid,
            'value' => $ancestor->tid,
          );
          $fields[] = array(
            'key' => 'im_vid_'. $ancestor->vid,
            'value' => $ancestor->tid,
          );
          $name = apachesolr_clean_text($ancestor->name);
          $vocab_names[$ancestor->vid][] = $name;
          // We index each name as a string for cross-site faceting
          // using the vocab name rather than vid in field construction .
          $fields[] = array(
            'key' => 'sm_vid_'. apachesolr_vocab_name($ancestor->vid),
            'value' => $name,
          );
        }
      }
      // Index the term names into a text field for MLT queries and keyword searching.
      foreach ($vocab_names as $vid => $names) {
        $fields[] = array(
          'key' => 'ts_vid_'. $vid .'_names',
          'value' => implode(' ', $names),
        );
      }
  }
  return $fields;
}

/**
 * Helper function - return a safe (PHP identifier) vocabulary name.
 */
function apachesolr_vocab_name($vid) {
  static $names = array();

  if (!isset($names[$vid])) {
    $vocab_name = db_query('SELECT v.name FROM {taxonomy_vocabulary} v WHERE v.vid = :vid', array(':vid' => $vid))->fetchField();
    $names[$vid] = preg_replace('/[^a-zA-Z0-9_\x7f-\xff]/', '_', $vocab_name);
    // Fallback for names ending up all as '_'.
    $check = rtrim($names[$vid], '_');
    if (!$check) {
      $names[$vid] = '_' . $vid . '_';
    }
  }
  return $names[$vid];
}

/**
 * Callback that converts list module field into an array
 */
function apachesolr_fields_list_indexing_callback($node, $field_name, $index_key, $field_info) {
  $fields = array();
  if (!empty($node->{$field_name})) {
    $field = $node->$field_name;
    list($lang, $values) = each($field);
    foreach ($values as $fval) {
      $fields[] = array(
        'key' => $index_key,
        'value' => apachesolr_clean_text($fval['value']),
      );
    }
  }
  return $fields;
}

function apachesolr_cck_nodereference_indexing_callback($node, $field_name, $index_key, $cck_info) {
  $fields = array();
  if (!empty($node->{$field_name})) {
    $index_key = apachesolr_index_key($cck_info);
    foreach ($node->$field_name as $field) {
      if ($index_value = (isset($field['nid']) && strlen($field['nid'])) ? $field['nid'] : FALSE) {
        $fields[] = array(
          'key' => $index_key,
          'value' => $index_value,
        );
      }
    }
  }
  return $fields;
}

function apachesolr_cck_userreference_indexing_callback($node, $field_name, $index_key, $cck_info) {
  $fields = array();
  if (!empty($node->$field_name)) {
    $index_key = apachesolr_index_key($cck_info);
    foreach ($node->{$field_name} as $field) {
      if ($index_value = (isset($field['uid']) && strlen($field['uid'])) ? $field['uid'] : FALSE) {
        $fields[] = array(
          'key' => $index_key,
          'value' => $index_value,
        );
      }
    }
  }
  return $fields;
}

/**
 * Extract HTML tag contents from $text and add to boost fields.
 *
 * $text must be stripped of control characters before hand.
 */
function apachesolr_add_tags_to_document(&$document, $text) {
  $tags_to_index = variable_get('apachesolr_tags_to_index', array(
    'h1' => 'tags_h1',
    'h2' => 'tags_h2_h3',
    'h3' => 'tags_h2_h3',
    'h4' => 'tags_h4_h5_h6',
    'h5' => 'tags_h4_h5_h6',
    'h6' => 'tags_h4_h5_h6',
    'u' => 'tags_inline',
    'b' => 'tags_inline',
    'i' => 'tags_inline',
    'strong' => 'tags_inline',
    'em' => 'tags_inline',
    'a' => 'tags_a'
  ));

  // Strip off all ignored tags.
  $text = strip_tags($text, '<'. implode('><', array_keys($tags_to_index)) .'>');

  preg_match_all('@<('. implode('|', array_keys($tags_to_index)) .')[^>]*>(.*)</\1>@Ui', $text, $matches);
  foreach ($matches[1] as $key => $tag) {
    $tag = strtolower($tag);
    // We don't want to index links auto-generated by the url filter.
    if ($tag != 'a' || !preg_match('@(?:http://|https://|ftp://|mailto:|smb://|afp://|file://|gopher://|news://|ssl://|sslv2://|sslv3://|tls://|tcp://|udp://|www\.)[a-zA-Z0-9]+@', $matches[2][$key])) {
      if (!isset($document->{$tags_to_index[$tag]})){
        $document->{$tags_to_index[$tag]} = '';
      }
      $document->{$tags_to_index[$tag]} .= ' '. apachesolr_clean_text($matches[2][$key]);
    }
  }
}

/**
 * Additional index utility functions
 */

/**
 * hook_cron() helper to try to make {apachesolr_search_node} consistent with {node}.
 * TODO: this function is never used
 */
function apachesolr_cron_check_node_table() {
  // Check for unpublished content that wasn't deleted from the index.
  $query = db_select('apachesolr_search_node', 'asn')
    ->fields('n', array('nid', 'status'))
    ->condition('asn.status', 'n.status', '<>');
  $query->innerJoin('node', 'n', 'n.nid = asn.nid');
  $result = $query->execute();

  $node_lists = array();
  $nodes = array();
  // Update or delete at most this many in each Solr query.
  $limit = variable_get('apachesolr_cron_mass_limit', 500);
  foreach ($result as $node) {
    $nodes[$node->nid] = $node;
    if (count($nodes) == $limit) {
      $node_lists[] = $nodes;
      $nodes = array();
    }
  }
  // Any remaning ones if the limit is not reached.
  if (count($nodes)) {
    $node_lists[] = $nodes;
  }
  foreach ($node_lists as $nodes) {
    watchdog('Apache Solr', 'On cron running apachesolr_nodeapi_mass_update() on nids @nids', array('@nids' => implode(',', array_keys($nodes))), WATCHDOG_WARNING);
    if (!apachesolr_nodeapi_mass_update($nodes)) {
      // Solr query failed - so stop trying.
      break;
    }
  }
  // Check for deleted content that wasn't deleted from the index.
  $query = db_select('apachesolr_search_node', 'asn')
    ->fields('asn', array('nid'))
    ->isNull('n.nid');
  $query->leftJoin('node', 'n', 'n.nid = asn.nid');
  $result = $query->execute();

  $node_lists = array();
  $nodes = array();
  foreach ($result as $node) {
    $nodes[$node->nid] = $node;
    if (count($nodes) == $limit) {
      $node_lists[] = $nodes;
      $nodes = array();
    }
  }
  // Any remaning ones if the limit is not reached.
  if (count($nodes)) {
    $node_lists[] = $nodes;
  }
  foreach ($node_lists as $nodes) {
    watchdog('Apache Solr', 'On cron running apachesolr_nodeapi_mass_delete() on nids @nids', array('@nids' => implode(',', array_keys($nodes))), WATCHDOG_WARNING);
    if (!apachesolr_nodeapi_mass_delete($nodes)) {
      // Solr query failed - so stop trying.
      break;
    }
  }
}

function apachesolr_nodeapi_mass_update($nodes) {
  if (empty($nodes)) {
    return TRUE;
  }
  $published_ids = array();
  $unpublished_ids = array();
  foreach ($nodes as $node) {
    if ($node->status) {
      $published_ids[$node->nid] = apachesolr_document_id($node->nid);
    }
    else {
      $unpublished_ids[$node->nid] = apachesolr_document_id($node->nid);
    }
  }
  try {
    $solr = apachesolr_get_solr();
    $solr->deleteByMultipleIds($unpublished_ids);
    apachesolr_index_set_last_updated(REQUEST_TIME);

      // There was no exception, so update the table.
    if ($published_ids) {
      db_update('apachesolr_search_node')->fields(array('changed' => REQUEST_TIME, 'status' => 1))->condition('nid', array_keys($published_ids), 'IN')->execute();
    }
    if ($unpublished_ids) {
      db_update('apachesolr_search_node')->fields(array('changed' => REQUEST_TIME, 'status' => 0))->condition('nid', array_keys($unpublished_ids), 'IN')->execute();
    }
    return TRUE;
  }
  catch (Exception $e) {
    watchdog('Apache Solr', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
    return FALSE;
  }
}

function apachesolr_nodeapi_mass_delete($nodes) {
  if (empty($nodes)) {
    return TRUE;
  }
  $ids = array();
  $nids = array();
  foreach ($nodes as $node) {
    $ids[] = apachesolr_document_id($node->nid);
    $nids[] = $node->nid;
  }
  try {
    $solr = apachesolr_get_solr();
    $solr->deleteByMultipleIds($ids);
    apachesolr_index_set_last_updated(REQUEST_TIME);
    // There was no exception, so update the table.
    db_delete('apachesolr_search_node')->condition('nid', $nids, 'IN')->execute();
    return TRUE;
  }
  catch (Exception $e) {
    watchdog('Apache Solr', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
    return FALSE;
  }
}
