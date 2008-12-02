<?php
require_once 'SolrPhpClient/Apache/Solr/Service.php';

class Drupal_Apache_Solr_Service extends Apache_Solr_Service {

  protected $luke;
  protected $luke_cid;
  const LUKE_SERVLET = 'admin/luke';

  /**
   * Sets $this->luke with the meta-data about the index from admin/luke.
   */
  protected function setLuke($num_terms = 0) {
    if (empty($this->luke[$num_terms])) {
      $url = $this->_constructUrl(self::LUKE_SERVLET, array('numTerms' => "$num_terms", 'wt' => self::SOLR_WRITER));
      $this->luke[$num_terms] = $this->_sendRawGet($url);
      cache_set($this->luke_cid, $this->luke);
    }
  }

  /**
   * Get just the field meta-data about the index.
   */
  public function getFields($num_terms = 0) {
    return $this->getLuke($num_terms)->fields;
  }

  /**
   * Get meta-data about the index.
   */
  public function getLuke($num_terms = 0) {
    if (!isset($this->luke[$num_terms])) {
      $this->setLuke($num_terms);
    }
    return $this->luke[$num_terms];
  }

  /**
   * Clear cached Solr data.
   */
  public function clearCache() {
    cache_clear_all("apachesolr:luke:", 'cache', TRUE);
    $this->luke = array();
  }

  /**
   * Clear the cache whenever we commit changes.
   *
   * @see Apache_Solr_Service::commit()
   */
  public function commit($optimize = TRUE, $waitFlush = TRUE, $waitSearcher = TRUE, $timeout = 3600) {
    parent::commit($optimize, $waitFlush, $waitSearcher, $timeout);
    $this->clearCache();
  }

  /**
   * Construct the Full URLs for the three servlets we reference
   *
   * @see Apache_Solr_Service::_initUrls()
   */
  protected function _initUrls() {
    parent::_initUrls();
    $this->_lukeUrl = $this->_constructUrl(self::LUKE_SERVLET, array('numTerms' => '0', 'wt' => self::SOLR_WRITER));
  }

  /**
   * Put Luke meta-data from the cache into $this->luke when we instantiate.
   *
   * @see Apache_Solr_Service::__construct()
   */
  public function __construct($host = 'localhost', $port = 8180, $path = '/solr/') {
    parent::__construct($host, $port, $path);
    $this->luke_cid = "apachesolr:luke:$host:$port:$path";
    $cache = cache_get($this->luke_cid);
    if (isset($cache->data)) {
      $this->luke = $cache->data;
    }
  }
}
