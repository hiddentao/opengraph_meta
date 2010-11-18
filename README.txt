$Id$
====================
Open Graph meta tags
====================


1. Introduction
---------------

This is a module which makes it easy to add Open Graph meta tags (http://opengraphprotocol.org/) to a node to enable it to be come a "rich" social object. For instance, Facebook uses this information to work out how to preview shared content in a user's Facebook profile (http://developers.facebook.com/docs/share).

This module makes it easy to select the image thumbnail used to represent the node (used by Facebook when constructing a preview). The editor is shown a list of thumbnails of all images associated with the node (both as fields as well as images embedded within the node's body content).

If need be, meta tags can be restricted to specific content types and access to administering and editing meta tags can be restricted by role using Drupal's built-in permissions system.

The latest code for this module is always available at http://github.com/hiddentao/opengraph_meta


2. Installation
---------------

Add the following XML namespace attribute to the HTML tag at the top of your page.tpl.php file:

xmlns:og="http://opengraphprotocol.org/schema/"

For example:

<html xmlns:og="http://opengraphprotocol.org/schema/">
