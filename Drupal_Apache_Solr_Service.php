<?php
require_once 'SolrPhpClient/Apache/Solr/Service.php';

class Drupal_Apache_Solr_Service extends Apache_Solr_Service {

  var $luke;
  const LUKE_SERVLET = 'admin/luke';

  /**
   * Sets $this->luke to the return from admin/luke which provides meta-data about in the index including fields
   *
   */
  protected function setLuke() {
    if (empty($this->luke)) {
      //@TODO: WE should actually use the vars for connection we instantiated with.
      $this->luke = $this->_sendRawGet($this->_lukeUrl);
    }
  }

  public function getFields() {
      if (!isset($this->luke->fields)) {
          $this->setLuke();
      }
      return $this->luke->fields;
  }

  /**
   * Construct the Full URLs for the three servlets we reference
   */
  protected function _initUrls()
  {
    parent::_initUrls();
    $this->_lukeUrl = $this->_constructUrl(self::LUKE_SERVLET, array('numTerms' => '0', 'wt' => self::SOLR_WRITER ));
  }

}
