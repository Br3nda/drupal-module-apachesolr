/* $Id: README.txt,v 1.4 2008/07/28 07:09:40 robertDouglass Exp $ */

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

Views integration
-----------------

At the current state of the Views integration for the module, there is a view that uses an apachesolr argument to search the site with Apache Solr. It's located at /solrsearch, just enable the corresponding menu item and you are ready to go.

There is also a block named "ApacheSolr: Search results" which will display the results of the current ApacheSolr search, if there was one. You can use this one to display a view containing the results in addtion to the normal display, disabling the default display is currently not possible.
