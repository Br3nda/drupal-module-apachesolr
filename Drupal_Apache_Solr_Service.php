<?php
require_once 'SolrPhpClient/Apache/Solr/Service.php';

class Drupal_Apache_Solr_Service extends Apache_Solr_Service {

  var $luke;

  protected function setLuke() {
    if (empty($this->luke)) {
      //@TODO: WE should actually use the vars for connection we instantiated with.
      $response = drupal_http_request(apachesolr_base_url() ."/admin/luke?numTerms=0&wt=json");
      if ($response->code == '200') {
        $this->luke = json_decode($response->data);
      } else {
        throw new Exception('ApacheSolr Failed to get data from LUKE got '.$response->code.' from '  . apachesolr_base_url() ."/admin/luke?numTerms=0&wt=json");
      }
    }
  }

  public function getFields() {
      if (!isset($this->luke->fields)) {
          $this->setLuke();
      }
      return $this->luke->fields;
  }
}
