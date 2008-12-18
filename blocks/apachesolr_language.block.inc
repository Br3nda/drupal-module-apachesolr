<?php
// $Id: apachesolr_language.block.inc,v 1.1.2.1 2008/12/18 09:29:43 jacobsingh Exp $

function apachesolr_language_block_view() {
  $title = t('ApacheSolr: Filter by language');
  if (apachesolr_has_searched()) {
    // Get the query and response. Without these no blocks make sense.
    $response = apachesolr_static_response_cache();
    if (empty($response)) {
      return;
    }
    $query = apachesolr_current_query();
    if (is_object($response->facet_counts->facet_fields->language)) {
      $contains_active = FALSE;
      $languages = array();
      foreach ($response->facet_counts->facet_fields->language as $language => $count) {
        $unclick_link = '';
        unset($active);
        $new_query = clone $query;
        if ($active = $query->has_field('language', $language)) {
          $contains_active = TRUE;
          $new_query->remove_field('language', $language);
          $path = 'search/' . arg(1) . '/' . $new_query->get_query_basic();
          $querystring = $new_query->get_url_querystring();
          $unclick_link = theme('apachesolr_unclick_link', $path, $querystring);
        }
        else {
          $new_query->add_field('language', $language);
          $path = 'search/' . arg(1) . '/' . $new_query->get_query_basic();
          $querystring = $new_query->get_url_querystring();
        }
        $countsort = $count == 0 ? '' : 1 / $count;
        // if numdocs == 1 and !active, don't add.
        if ($response->numFound == 1 && !$active) {
          // skip
        }
        else {
          $languages[] = theme('apachesolr_facet_item', locale_language_name($language), $count, $path, $querystring, $active, $unclick_link);
        }`
      }

      if (count($languages) > 0) {
        $facet_display_limits = variable_get('apachesolr_facet_query_limits', array());
        $facet_display_limit = isset($facet_display_limits[$delta]) ? $facet_display_limits[$delta] : 10;
        $languages = array_slice($languages, 0, $facet_display_limit);
        $output = theme('apachesolr_facet_list', $languages);
        return array('subject' => $title, 'content' => $output);
      }
    }
  }
}
