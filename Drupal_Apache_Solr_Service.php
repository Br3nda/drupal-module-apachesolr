<?php
require_once 'SolrPhpClient/Apache/Solr/Service.php';

class Drupal_Apache_Solr_Service extends Apache_Solr_Service {

  var $luke;
  const LUKE_SERVLET = 'admin/luke';

  /**
   * Sets $this->luke to the return from admin/luke which provides meta-data about in the index including fields
   *
   */
  protected function setLuke($num_terms = 0) {
    if (empty($this->luke[$num_terms])) {
      $url = $this->_constructUrl(self::LUKE_SERVLET, array('numTerms' => "$num_terms", 'wt' => self::SOLR_WRITER));
      $this->luke[$num_terms] = $this->_sendRawGet($url);
    }
  }

  public function getFields($num_terms = 0) {
    return $this->getLuke($num_terms)->fields;
  }

  public function getLuke($num_terms = 0) {
    if (!isset($this->luke[$num_terms])) {
        $this->setLuke($num_terms);
    }
    return $this->luke[$num_terms];
  }

  /**
   * Construct the Full URLs for the three servlets we reference
   */
  protected function _initUrls()
  {
    parent::_initUrls();
    $this->_lukeUrl = $this->_constructUrl(self::LUKE_SERVLET, array('numTerms' => '0', 'wt' => self::SOLR_WRITER));
  }

}
