/* $Id: README.txt,v 1.1.2.1.2.3 2008/10/05 17:29:15 robertDouglass Exp $ */

This module integrates Drupal with the Apache Solr search platform. Solr search can be used as a replacement for core content search and boasts both extra features and better performance. Among the extra features is the ability to have faceted search on facets ranging from content author to taxonomy to arbitrary CCK fields.

The module comes with a schema.xml file which should be used in your Solr installation.

This module depends on the search framework in core. However, you may not want the core searches and only want Solr search. If that is the case, you want to use the Core Searches module in tandem with this module.


Installation
------------

Install and enable the ApacheSolr Drupal module as you would any Drupal module.

Prerequisite: Java 5 or higher.

Download Solr 1.2 or higher from a mirror site:
http://www.apache.org/dyn/closer.cgi/lucene/solr/

Unpack the tarball somewhere not visible to the web (not in your apache docroot and not inside of your drupal directory).

The Solr download comes with an example application that you can use for testing, development, and even for smaller production sites. This application is found at apache-solr-1.2.x/example.

Move apache-solr-1.2.x/example/solr/conf/schema.xml and rename it to something like schema.bak. Then move the schema.xml that comes with the ApacheSolr Drupal module to take its place.

Now start the solr application by opening a shell, changing directory to apache-solr-1.2.x/example, and executing the command java -jar start.jar

Test that your solr server is now available by visiting http://localhost:8983/solr/admin/

Now run cron on your Drupal site until your content is indexed.

Enable blocks for facets at Administer > Site building > Blocks.   

Troubleshooting
--------------
Problem:
Your Solr instance is running and you can test it in the Solr 
admin interface (comes with the Java application). Yet your 
Drupal ApacheSolr module cannot connect to it to do a search.

Solution:
To be able to use file_get_contents() in PHP, the "allow_url_fopen" 
directive must be enabled. In php.ini set the following value:
allow_url_fopen = On


Developers
--------------

Exposed Hooks:

@param &$document Apache_Solr_Document
@param $node StdClass
hook_apachesolr_update_index(&$document, $node)

This hook is called just before indexing the document.
It allows you to add fields to the $document object which is sent to Solr.
For reference on the $document object, see SolrPhpClient/Apache/Solr/Document.php
