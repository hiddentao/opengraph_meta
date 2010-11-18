<?php
// $Id$

define('OPENGRAPH_META_TABLE','opengraph_meta');

define('OPENGRAPH_META_PERM_ADMIN', 'administer Open Graph meta tags');
define('OPENGRAPH_META_PERM_EDIT', 'edit Open Graph meta tags');

define('OPENGRAPH_META_VAR_CONTENT_TYPES_ENABLED', 'opengraph_meta_types_enabled');
define('OPENGRAPH_META_VAR_CONTENT_TYPE_', 'opengraph_meta_type_');
define('OPENGRAPH_META_VAR_SITE_NAME', 'opengraph_meta_site_name');



class OpenGraphMeta {

  const TITLE = 'title';
  const DESCRIPTION = 'description';
  const IMAGE = 'image';
  const SITE_NAME = 'site_name';
  const TYPE = 'type';

  /**
   * Get default values for all meta tags.
   * @static
   * @param $node a node object. If provided then defaults will be tailored to this node.
   */
  private static function get_og_tag_defaults($node = NULL) {
    // defaults
    static $defaults = array();
    if (empty($defaults)) {
      $defaults = array(
        self::TITLE => '',
        self::DESCRIPTION => '',
        self::IMAGE => '',
        self::SITE_NAME => variable_get(OPENGRAPH_META_VAR_SITE_NAME, variable_get('site_name','Drupal')),
        self::TYPE => '',
      );
    }

    if (empty($node)) {
      // if no node given then return cached defaults
      return $defaults;
    } else {
      // if node given then override defaults
      $ret = $defaults;
      $ret[self::TITLE] = $node->title;
      $ret[self::DESCRIPTION] = !empty($node->body) ? mb_substr(strip_tags($node->body),0,200) : $node->title;
      $ret[self::TYPE] = variable_get(OPENGRAPH_META_VAR_CONTENT_TYPE_.$node->type,'');
      $image_paths = self::harvest_images_from_node($node);
      $ret[self::IMAGE] = !empty($image_paths) ? array_shift($image_paths) : '';
      return $ret;
    }
  }


  /**
   * Get all possible values for the og:type meta tags, grouped by category, ready for use as #options for a
   * SELECT form item.
   */
  public static function get_all_og_types_for_select_field() {
    static $ret = array();
    if (empty($ret)) {
      // Taken from http://opengraphprotocol.org/ on 18/Nov/2010
      $ogtypes = array(
        t('Activities') => array('activity','sport'),
        t('Businesses') => array('bar','company','cafe','hotel','restaurant'),
        t('Groups') => array('cause','sports_league','sports_team'),
        t('Organizations') => array('band','government','non_profit','school','university'),
        t('People') => array('actor','athlete','author','director','musician','politician','public_figure'),
        t('Places') => array('city','country','landmark','state_province'),
        t('Products and Entertainment') => array('album','book','drink','food','game','movie','product','song','tv_show'),
        t('Websites') => array('article','blog','website'),
      );
      $ret = array('' => '');
      foreach ($ogtypes as $cat => $t) {
        $ret[$cat] = array_combine($t,$t);
      }
    }
    return $ret;
  }


  /**
   * Get whether meta tags are enabled for the given content type.
   * @return TRUE if so; FALSE otherwise.
   */
  public static function tags_are_enabled_for_content_type($type) {
    $content_types = variable_get(OPENGRAPH_META_VAR_CONTENT_TYPES_ENABLED, array());
    $content_types = array_filter($content_types);
    // if no content types specifically set OR if this content type is set then tags are enabled
    return empty($content_types) || !empty($content_types[$type]);
  }



  /** Delete FB meta tag data for the given node. */
  public static function delete_node_data($nid) {
    db_query("DELETE FROM {".OPENGRAPH_META_TABLE."} WHERE nid = %d", $nid);
  }

  /**
   * Save FB meta tag data for the given node.
   *
   * @param $data key-value pairs
   */
  public static function save_node_data($nid, $data) {
    $ret = db_fetch_object(db_query("SELECT * FROM {".OPENGRAPH_META_TABLE."} WHERE nid = %d", $nid));

    if (FALSE === $ret) {
      $row = new stdClass();
      $row->nid = $nid;
    }
    else {
      $row = $ret;
    }

    foreach (self::get_og_tag_defaults() as $field => $_d) {
      $row->$field = $data[$field];
    }
    drupal_write_record(OPENGRAPH_META_TABLE, $row, FALSE !== $ret ? 'nid' : array());
  }

  /**
   * Load FB meta tag data for the given node.
   *
   * @return array('title' => ..., 'summary' => ...)
   */
  public static function load_node_data($node) {
    $result = self::get_og_tag_defaults();

    $row = db_fetch_object(db_query("SELECT * FROM {".OPENGRAPH_META_TABLE."} WHERE nid = %d", $node->nid));
    if (FALSE !== $row) {
      foreach ($result as $field => &$val) {
        $val = $row->$field;
      }
    }

    return $result;
  }

  /** Render the meta tag data for the given node. */
  public static function render_data($node, $opengraph_data) {
    // fallback values in case no values set.
    $allfields = self::get_og_tag_defaults($node);
    foreach ($allfields as $field => $_d) {
      $v = !empty($opengraph_data[$field]) ? $opengraph_data[$field] : $_d;
      if (!empty($v)) {
        switch ($field) {
          case self::IMAGE:
            $v = url($v, array('absolute' => TRUE));
            break;
        }
        drupal_set_html_head("<meta property=\"og:{$field}\" content=\"{$v}\" />");
      } // if value available for field
    } // for each field
  }


  /**
   * Harvest all images from the given node.
   *
   * @return array(absolute image URl)
   */
  public static function harvest_images_from_node($node) {
    // extract image fields
    $ret = array();
    self::_extract_image_fields_from_node((array)$node, $ret);

    // extract all images from body content
    if (!empty($node->body)) {
      $doc = new DOMDocument();
      $doc->loadHTML($node->body);
      $list = $doc->getElementsByTagName('img');
      for ($i=0; $list->length > $i; ++$i) {
        $ret[] = $list->item($i)->getAttribute('src');
      }
    }

    return $ret;
  }

  /**
   * Helper to harvest_images_from_node().
   *
   * array_walk_recursive() doesn't give us enough flexibility so we do the recursion manually.
   */
  private static function _extract_image_fields_from_node($val, array &$resultarray) {
    if (is_array($val)) {
      if (!empty($val['filemime']) && 0 == stripos($val['filemime'], 'image') && !empty($val['filepath'])) {
        array_push($resultarray, $val['filepath']);
      }
      else {
        foreach ($val as $cv) {
          self::_extract_image_fields_from_node($cv, $resultarray);
        }
      }
    }
  }

}
