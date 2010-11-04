<?php
// $Id$

define('OPENGRAPH_META_TABLE','opengraph_meta');

define('OPENGRAPH_META_PERM_ADMIN', 'administer Open Graph meta tags');
define('OPENGRAPH_META_PERM_EDIT', 'edit Open Graph meta tags');

define('OPENGRAPH_META_VAR_CONTENT_TYPES', 'opengraph_meta_content_types');



class OpenGraphMeta {

  const TITLE = 'title';
  const DESCRIPTION = 'description';
  const IMAGE = 'image';

  private static $defaults = array(
    self::TITLE => '',
    self::DESCRIPTION => '',
    self::IMAGE => '',
  );


  /**
   * Get whether meta tags are enabled for the given content type.
   * @return TRUE if so; FALSE otherwise.
   */
  public static function tags_are_enabled_for_content_type($type) {
    $content_types = variable_get(OPENGRAPH_META_VAR_CONTENT_TYPES, array());
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

    foreach (self::$defaults as $field => $_d) {
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
    $result = self::$defaults;

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
    $fallbacks = array();
    $fallbacks[self::TITLE] = $node->title;
    $fallbacks[self::DESCRIPTION] = !empty($node->body) ? mb_substr(strip_tags($node->body),0,200) : $node->title;
    $image_paths = self::harvest_images_from_node($node);
    $fallbacks[self::IMAGE] = !empty($image_paths) ? array_shift($image_paths) : '';

    foreach (self::$defaults as $field => $_d) {
      $v = !empty($opengraph_data[$field]) ? $opengraph_data[$field] : $fallbacks[$field];
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
   * Helper to harves_images_from_node().
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
