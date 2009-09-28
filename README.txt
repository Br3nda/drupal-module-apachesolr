/* $Id: README.txt,v 1.1.2.6 2009/09/28 22:08:14 damienvancouver Exp $ */

Apache Solr Search Integration 5.x-1.0

REAMDE Contents:
   Introduction
   Prerequisite Installations
     Sun Java 5 + Runtime Environment
     Apache Solr 1.3.x     
       Simple install option: Standalone Solr run with a java command 
       Advanced/server install option: TomCat Solr Installation run as a service
   Drupal Module Installation and Configuration
   Enable Facet blocks
   (Optional) Disable Core Search functionality with coresearches
   (Optional) Enable Node Access Support
   (Optional, Advanced) Enable Attachment Support
   Re-Index your site and run cron.php to build the index
   Sharing one Solr instance shared between many Drupal sites
   Troubleshooting
   Developer Information


Introduction
------------

This module integrates Drupal with the Apache Solr search platform. In Drupal, Solr search can be used as a replacement for core content search and boasts both extra features and better performance.  Among the extra features are the ability to have faceted search on facets ranging from content author to taxonomy to arbitrary CCK fields, and the abillity to index and "search inside" Acrobat (.pdf), Word 97-2004 (.doc), Word 2007+ (.docx), and Plain Text (.txt) attachments.

For more introductory information on Solr, visit its wikipedia entry and home page:
  http://lucene.apache.org/solr/
  http://en.wikipedia.org/wiki/Apache_Solr  

This module comes with a schema.xml file which should be used in your Solr installation.  Follow the installation steps below carefully, as the module will not just work out of the box with a default Solr configuration.

This module depends on the search framework in core.  By default, you get results from the core search functionality for content and uesrs, as well as Solr results.  If you you do not want the core search capabilities and only want Solr results returned, you need to install the Core Searches module in tandem with this module.  See the sectoin "(Optional) Disable Core Search functionality with coresearches", below.

In order to successfully set this all up on a production site, you probably need administrator access to your server, or at least cooperation/help from your system's administrator to get TomCat and Solr configured properly.


Prerequisite Installations
--------------------------

Prerequisite: Sun Java 5 + Runtime Environment  
Ideally you should run the official Sun Java JRE and not
one of the various open source alternatives.    

On Ubuntu Linux, make sure "multiverse" is enabled in System->Administration->Sofwtare Sources, then install the "sun-java6-jre" or "sun-java6-jdk" packages.  After installation use the command: "sudo update-java-alternatives -s java-6-sun" to set the Sun JRE as default.  Finally, use "java -version" to make sure you are running the official Java(TM) from Sun.

On Debian Linux, you need to enable "non-free" in your /etc/apt/sources.list and then install the sun-java5-jre package.   After installation use "sudo /usr/sbin/alternatives --config java" to set the Sun JRE as the default. 

For RedHat/Fedora Core/CentOS, install the java-1.6.0-sun package, or see http://wiki.centos.org/HowTos/JavaOnCentOS for instructions to create it if it is not available.  After installation use "sudo /usr/sbin/alternatives --config java" to set the Sun JRE as the default.

for Windows, download the correct JRE and install it for your version of windows and install it.

Once Java is correctly installed, the command "java -version" should show the Sun Java(TM) Header, e.g.

  $ java -version
  java version "1.6.0_16"
  Java(TM) SE Runtime Environment (build 1.6.0_16-b01)
  Java HotSpot(TM) 64-Bit Server VM (build 14.2-b01, mixed mode)


Prerequisite: Apache Solr 1.3.x:

Download the latest Solr 1.3.x release from a mirror site:
http://www.apache.org/dyn/closer.cgi/lucene/solr/

At 1.0 release time, the latest version was apache-solr-1.3.0.tgz.

Unpack the apache_solr-1.3.0.tgz somewhere not visible to the web (not in your apache docroot and not inside of your drupal directory). 

$ tar xzvf apache_solr-1.3.0.tgz

The Solr download comes with an example instance (named "example") that you can use as-is for testing, development, and even for smaller production sites. This instance is found in the apache-solr-1.3.0/example folder.  These instructions assume you are going to use that "example" application to run Solr (the simplest way to get it working quickly).  In production, you may wish to move/copy the "example" folder to a new location.  (e.g. cp solr-1.3.0/example /home/solr).  In that case, adjust your paths 

First rename the stock schema.xml file that comes with the solr example instance:
rename apache-solr-1.3.0/example/solr/conf/schema.xml to something like schema.bak.
 
Now copy the schema.xml that comes with the ApacheSolr Drupal module to take its place in solr/conf:
(e.g. cp /var/www/drupal/sites/all/modules/apachesolr/schema.xml solr-1.3.0/example/solr/conf/)

There are two ways to run Solr, as a standalone application, or inside an enterprise servlet container such as Apache Tomcat.   Standalone is much eaiser to set up, and more suitable for local development and testing, while Tomcat is much harder to set up, but more suitable for use on production webservers.  In standalone mode, Solr only runs as long as the window with the java command is open.  With Tomcat, Solr can be delivered as an always-on service (but again, it's much harder to set up).


Simple install option: Standalone Solr run with a java command:


You can easily start Solr by opening a shell/Terminal Window, changing the directory to apache-solr-1.3.0/example, and executing the command: "java -jar start.jar"

There should be lots of terminal output, ending with something like this:

  INFO: [] Registered new searcher Searcher@5a20f443 main
  2009-09-28 09:17:17.948::INFO:  Started SocketConnector @ 0.0.0.0:8983

That last line says that Solr should now be listening on port 8983 of your computer.

Test this in a web browser by visiting http://<your server address>:8983/solr/admin/, 
(e.g. http://localhost:8983/solr/admin/ )

  Any time you reboot the computer or close the terminal window, you will have to change to the directory and run java -jar start.jar again to start Solr.   You can keep the standalone one running even if you accidentally close the terminal window by using the screen program.  Install screen, then "screen java -jar start.jar" to start the server.  If you close the window, you can re-attach later with "screen -d -r").  This method will not auto-start Solr if the computer reboots, if you need that consider moving up to the Tomcat install.


Advanced/server install option: TomCat Solr Installation run as a service:

Installing TomCat is a subject that varies from operating system to operating system.  You should search for appropriate instructions for your OS to get TomCat working to the point where you can  see the "It works!" page at http://<your server address>:8080.  

In Ubuntu Linux, the latest version at time of this release is tomcat6 and should use sun-java6-jre.  Tomcat6 listens on port 8180 by default.  Debian Lenny uses tomcat5.5 (and should work best with sun-java5-jre).  Tomcat listens on port 8080 by default.

For Fedora / Redhat / CentOS, you may be able to install java-1.6.0-sun package, or you might have to download the RPM package from Sun at java.com and install it manually.


In either case, after Tomcat is installed, it will have a configuration directory containing a subdirectory named "Catalina", with another subdirectory "localhost".  The location of this tomcat configuration directory may change depending on your OS version. In Ubuntu Linux the default configuration directory is "/etc/tomcat6/".  In  Debian Linux the default configuration directory is "/etc/tomcat5.5//"

Create a file in the Catalina/localhost directory called "solr.xml", (e.g. /etc/tomcat6/Catalina/localhost/solr.xml)

If you renamed/moved the "example" instance to a more permanent name, put the proper path and name in solr.xml here!

<Context docBase="/home/myusername/solr-1.3.0/solr/webapps/solr.war" debug="0" crossContext="true">
<Environment name="solr/home" type="java.lang.String" value="/home/myusername/apache-solr-1.3.0/example/solr" override="true"  />
</Context>

After solr.xml is created, you need to turnoff the Java Security Manager (easy) or properly set permissions the solr instance needs in your java environment (hard, not covered here).
Edit /etc/default/tomcat6 and set TOMCAT6_SECURITY=no. 

Now restart Tomcat (sudo /etc/init.d/tomcat6 restart)

Try loading the Solr web administration interface in a web browser:

  http://<your server address>:8080/solr/admin  

If you see a Solr page, you have it working, congratulations!

If you see pages of large print Java exception errors, something is configured wrong.  Try reading through and look for what is wrong.  If it's a permission denied error, make sure you turned off the security manager in TomCat.  If it's complaining about not finding the Solr directory, make sure you set the correct path in solr.xml.  Failing those two, check the Tomcat logs (by default: /var/log/catalina.YYYY-MM-DD.log) and you may find more readable error output.

If you get a server not found, try port 8180 instead of 8080, and otherwise verify that Tomcat is running and you can see the Tomcat "It works!" default page.


Drupal Module Installation and Configuration
--------------------------------------------

Once Solr is running and you can see the administration interface, you can enable and configure the Drupal module.

Install and enable the ApacheSolr Drupal module as you would any Drupal module.  Unpack it to sites/all/modules and visit Administer > Site Building > Modules.  Enable at least "Apache Solr Framework" and
"Apache Solr Search".

Next, configure the module at Administer > Site Configuratin > Apache Solr > Settings.

Set the host name and port of your Solr instance.  Usually localhost will do for a host name, but
the port must match what port you're using:  8983 for standalone, 8080 for Ubuntu Tomcat 5 or 6, and 8180 for Tomcat 6 on Debian Lenny.

The Solr path should be left at the default "/solr".  Now "Save configuration" and if the ApacheSolr module can talk to Solr, a green message will say "Solr can be pinged".  If a red message says "No Solr instance is available", check that Solr is running (you can hit the Solr admin web page in a browser), correct the settings, and try again, until you see the message that Solr can be pinged.


Enable Facet blocks
-------------------
Enable blocks for facets at Administer > Site building > Blocks.   
Turning these blocks on gives users the ability to refine their searches using Faceted Search 
(for more info on faceted searching see: http://en.wikipedia.org/wiki/Faceted_search)


(Optional) Disable Core Search functionality with coresearches
--------------------------------------------------------------
Download the core searches, apply the latest patch for your Drupal version as described in the coresearches README.txt, and then enable Core Search and User Search modules to turn on those search features.  

$ cd /path/to/your/drupal
$ patch -p0 < sites/all/modules/coresearches/DRUPAL-5.20.patch  (you may have to fetch the latest patch for Drupal from http://drupal.org/node/353009)

WARNING - When you update your Drupal version (say for a security update), you will need to re-patch with the latest coresearches patch or you may get a white screen of death!  For this reason you should probably not install coresearches unless you are comfortable with the concept of re-patching your Drupal core files every upgrade.


(Optional) Enable Node Access Support
-------------------------------------
Visit Administer > Site Building > Modues and turn on apachesolr_nodeaccess.
You must re-index the site to see the permissions changes.

If you do not turn on Node Access support, then all content on the site will be returned in Solr search results.  That would probably be fine for a public site, but bad for a private intranet site with private groups.


(Optional, Advanced) Enable Attachment Support
----------------------------------------------
With attachment support, Solr can index the contents of PDF files, Text Files, and Word Documents that are saved as node attachments.

DO NOT JUST TURN ON ATTACHMENT SUPPORT without configuring it properly.  If you don't need it, leave it off!


Enable the apachesolr_attachments module at Administer > Site Building > Modules.
Visit Administer > Site Configuration > Apache Solr > Apache Solr Attachments Settings.

In this 5.x-1.x version, document contents are parsed by several helper applications.  
At least one of these helper applications must be built from source (doctotext).  

You need each the following programs on your system:

"pdftotext" for .PDF files (found in package "poppler-utils" or maybe "xpdf-utils" for Linux)
"catdoc" for .DOC files (found in package "catdoc" for Linux)
"doctotext" for .DOCX" files (install from source using instructions below)
  - download the source package from http://silvercoders.com/index.php?page=DocToText 
  - un tar it, run "make", and it should build you:  build/doctotext
  - test it by running "build/doctotext" with no arguments, it should print help
  - copy that to your path, i.e.  cp build/doctotext /usr/local/bin
  
You can choose not to process one or more of these file types by leaving the entry blank on the Apache Solr Attachments Settings page.

On Debian and Ubuntu the most secure settings are as follows, with doctotext installed in /usr/local/bin:

PDF helper:  /usr/bin/pdftotext "%file%" -
Text helper: /bin/cat "%file%"
MS Word (up to 2004) .doc helper:  /usr/bin/catdoc -a "%file%"
MS Word (2007+) .docx helper:  /usr/local/bin/doctotext "%file%"

Save the configuration.  You have to rebuild the index if you have not already (see below) in
order to see contents inside your attachments.   New attachments will be indexed automatically when cron.php next runs.


Re-Index your site and run cron.php to build the index
-------------------------------------------------------
Usually any new content is added to to the Solr index when cron.php runs.  Becuase we are installing from scratch, the Solr index is empty and you need to build the index by manually running cron.php until the index is 100% built.  (This is especially important if you are installing on a site with lots of existing content otherwise it could take hours or even days for it all to show up in search results).

First wipe the old index, in case cron.php has already started indexing content with the wrong settings (while you were configuring everything):

Visit "Administer > Site Configuration > Search Settings".  Press "Re-index" site and confirm.  The Solr index will be wiped.

Repeatedly run your site's cron.php page in another tab or window until your content is indexed.  cron.php will process 100 nodes per run by default.   After each cron.php run, refresh the admin/settings/search page so you can see what Percentage of the site is indexed. 

Once the Search Settings page reports that 100% of your site content is indexed, you are ready to try some test searches,

Use this procedure if you ever need to re-index everything on the site.  


Sharing one Solr instance shared between many Drupal sites
----------------------------------------------------------
One Solr install can be shared between as many sites as you like, with no special configuration required.

The Drupal apachesolr modules index a signature of the Site URL along with each piece of content content.  Thus a site's search will only return results for its own data, even if you have many sites indexed using the same Solr instance (ie. address, port, and path in ApacheSolr Settings).  

It is also possible to search across all of the sites that are indexed in your Solr, which is known as "Multi-Site search".

To use multi-site search, you must activate the Apache Solr multisite search module, and then searches will search across all sites in your instance.  Note that this is an all-or-nothing feature, you can't turn on multi site searching and only search -some- of the sites in Solr.  If you need to do this, consider running multiple Solr cores, which will give you moe than one instance.  You can then use one for the searchable sites, and one for sites that must not show up in the multi site search.   Finally, multi site searching may produce crazy results when combined with the apache solr node_access module.



Troubleshooting
---------------

Problem: Solr will not run in Tomcat, instead there are just pages of Java errors about security.permissionDenied.

Solution:  Make sure the security manager is disabled in your TomCat policy file.  Edit /etc/default/tomcat6 and set TOMCAT6_SECURITY=no , then restart Tomcat.  See the Tomcat Insatllation instructions above for more help.



Problem:
Your Solr instance is running and you can test it in the Solr 
admin interface (comes with the Java application). Yet your 
Drupal ApacheSolr module cannot connect to it to do a search.

Solution:
To be able to use file_get_contents() in PHP, the "allow_url_fopen" 
directive must be enabled. In php.ini set the following value:
allow_url_fopen = On
Only set this value if you are having problems, as it is against best security practices to do so.



Problem:
I've installed the module but the search resluts are incomplete, and I get only one or two results back from my site.

Solution:
Visit Administer -> Site Configuration > Search Settings and make sure your site is 100% indexed.  If not, run cron.php until it is.  See "Re-Index your site and run cron.php to build the index" above for more information.



Problem:
Administer -> Site Configuration > Search Settings says the site is 100% indexed, but no search results come back.

Solution:
Make sure that cron.php is being called from the EXACT site URL you are using to search.   ApacheSolr will use the URL of the cron.php call to determine the site name.  So for example, if your site is vieuwed at "http://www.example.com' and you call cron as "http://example.com/cron.php" or "http://localhost/cron.php" then ApacheSolr will index the wrong URL and you will not get site results!   Additionally, and for this reason, you should make sure you force users to end up at one definitive URL, with or without the www. prefix.  (One simple way to do this is setting $base_url in the settings.php file, so as soon as users click on anything or log in they will have the correct URL.  A better but much more advanced way is to use an Apache RewriteRule to redirect users as soon as they arrive.)

If that is not the problem, you should try typing search terms into the ApacheSolr admin interface in the large text box, and hitting Search.  You should be able to see results for simple one word searches that match your content.


Problem:
Anonymous users and users without permission are seeing protected content come back in search results, even though they do not have permissions!

Solution:
Ensure that the apachesolr_node_access module is activated.  Then visit Administer > Site Configuration > Search Settings and re-index the site.  Finally, run cron.php until the Search Settings page reports 100% of the content is indexed.  Now test again from anonymous and unprivelaged users.  Solr should only show results for which the current user has permission.



Developer Information
---------------------

Exposed Hooks:

@param &$document Apache_Solr_Document
@param $node StdClass
hook_apachesolr_update_index(&$document, $node)

This hook is called just before indexing the document.
It allows you to add fields to the $document object which is sent to Solr.
For reference on the $document object, see SolrPhpClient/Apache/Solr/Document.php 