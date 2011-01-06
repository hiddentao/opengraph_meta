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

  /** Singleton instance. */
  private static $instance = NULL;

  private $settings_obj = NULL;
  private $render_obj = NULL;
  private $data_obj = NULL;

  /**
   * Constructor
   */
  private function __construct() {
    $this->settings_obj = new OGMDrupalSettings();
    $this->render_obj = new OGMDrupalRender();
    $this->data_obj = new OGMDrupalData();
  }

  /** Get singleton instance. */
  public static function instance() {
    if (empty(self::$instance)) {
      $instance = new OpenGraphMeta();
    }
    return $instance;
  }

  /**
   * Get default values for all meta tags.
   * @param $node a node object. If provided then defaults will be tailored to this node.
   */
  private function get_og_tag_defaults($node = NULL) {
    // defaults
    static $defaults = array();
    if (empty($defaults)) {
      $defaults = array(
        self::TITLE => '',
        self::DESCRIPTION => '',
        self::IMAGE => '',
        self::TYPE => '',
      );
    }
 
    $defaults[self::SITE_NAME] = $this->settings_obj->get(OPENGRAPH_META_VAR_SITE_NAME, $this->settings_obj->get('site_name','Drupal'));

    if (empty($node)) {
      // if no node given then return cached defaults
      return $defaults;
    } else {
      // if node given then override defaults
      $ret = $defaults;
      $ret[self::TITLE] = $node->title;
      $ret[self::DESCRIPTION] = !empty($node->body) ? mb_substr(strip_tags($node->body),0,200) : $node->title;
      $ret[self::TYPE] = $this->settings_obj->get(OPENGRAPH_META_VAR_CONTENT_TYPE_.$node->type,'');
      $image_paths = $this->harvest_images_from_node($node);
      $ret[self::IMAGE] = !empty($image_paths) ? array_shift($image_paths) : '';
      return $ret;
    }
  }


  /**
   * Get all possible values for the og:type meta tags, grouped by category, ready for use as #options for a
   * SELECT form item.
   */
  public function get_all_og_types_for_select_field() {
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
  public function tags_are_enabled_for_content_type($type) {
    $content_types = $this->settings_obj->get(OPENGRAPH_META_VAR_CONTENT_TYPES_ENABLED, array());
    $content_types = array_filter($content_types);
    // if no content types specifically set OR if this content type is set then tags are enabled
    return empty($content_types) || !empty($content_types[$type]);
  }



  /** Delete FB meta tag data for the given node. */
  public function delete_node_data($nid) {
    $this->data_obj->delete_tags($nid);
  }

  /**
   * Save FB meta tag data for the given node.
   *
   * @param $data key-value pairs
   */
  public function save_node_data($nid, $data) {
    $ret = $this->data_obj->load_tags($nid);

    if (FALSE === $ret) {
      $row = new stdClass();
      $row->nid = $nid;
    }
    else {
      $row = $ret;
    }

    foreach ($this->get_og_tag_defaults() as $field => $_d) {
      $row->$field = isset($data[$field]) ? $data[$field] : '';
    }

    $this->data_obj->update_tags($row, FALSE !== $ret ? 'nid' : array());
  }

  /**
   * Load FB meta tag data for the given node.
   *
   * @return array('title' => ..., 'summary' => ...)
   */
  public function load_node_data($node) {
    $fields = $this->get_og_tag_defaults();

    $row = $this->data_obj->load_tags($node->nid);
    if (FALSE !== $row) {
      foreach ($fields as $field => &$val) {
        if (isset($row->$field))
          $val = $row->$field;
      }
    }

    return $fields;
  }

  /** Render the meta tag data for the given node. */
  public function render_data($node, $opengraph_data) {
    // fallback values in case no values set.
    $allfields = $this->get_og_tag_defaults($node);
    foreach ($allfields as $field => $_d) {
      $v = !empty($opengraph_data[$field]) ? $opengraph_data[$field] : $_d;
      if (!empty($v)) {
        switch ($field) {
          case self::IMAGE:
            $v = url($v, array('absolute' => TRUE));
            break;
          case self::DESCRIPTION:
            $v = htmlspecialchars(htmlspecialchars_decode($v));
            break;
        }
        $this->render_obj->add_meta('og:'.$field, $v);
      } // if value available for field
    } // for each field
  }


  /**
   * Harvest all images from the given node.
   *
   * @return array(image URl)
   */
  public function harvest_images_from_node($node) {
    // extract image fields
    $ret = array();
    $this->_extract_image_fields_from_node((array)$node, $ret);

    // extract all images from body content
    if (!empty($node->body)) {
      libxml_use_internal_errors(TRUE); // turn off libxml errors for now
      $doc = new DOMDocument();
      $doc->loadHTML($node->body);
      $list = $doc->getElementsByTagName('img');
      for ($i=0; $list->length > $i; ++$i) {
        $item = $list->item($i);
        if ($item->hasAttribute('src')) {
          $attrval = $item->getAttribute('src');
          if (!empty($attrval))
            $ret[] = $attrval;
        }
      }
      libxml_use_internal_errors(FALSE); // turn libxml errors back on
    }

    return $ret;
  }

  /**
   * Helper to harvest_images_from_node().
   *
   * array_walk_recursive() doesn't give us enough flexibility so we do the recursion manually.
   */
  private function _extract_image_fields_from_node($val, array &$resultarray) {
    if (is_array($val)) {
      if (!empty($val['filemime']) && FALSE !== stripos($val['filemime'], 'image') && !empty($val['filepath'])) {
        array_push($resultarray, $val['filepath']);
      }
      else {
        foreach ($val as $cv) {
          $this->_extract_image_fields_from_node($cv, $resultarray);
        }
      }
    }
  }

  /**
   * FOR TESTING PURPOSES ONLY!
   * Replace the internally used data and config instances with the given ones.
   */
  public function __set_objects($data_obj, $settings_obj, $render_obj) {
    $this->data_obj = $data_obj;
    $this->settings_obj = $settings_obj;
    $this->render_obj = $render_obj;
  }

  /**
   * FOR TESTING PURPOSES ONLY!
   * Get the internally used data and config instances with the given ones.
   */
  public function __get_objects() {
    return array($this->data_obj, $this->settings_obj, $this->render_obj);
  }

}


/** Interface to getting/setting config settings. */
interface OGMSettings {
  public function get($name, $default);
  public function set($name, $value);
}
/** Implementation which uses Drupal's variables store. */
class OGMDrupalSettings implements OGMSettings {
  public function get($name, $default) {
    return variable_get($name, $default);
  }
  public function set($name, $value) {
    variable_set($name, $value);
  }
}


/** Interface to affect page rendering. */
interface OGMRender {
  public function add_meta($name, $content);
}
/** Implementation which uses Drupal's methods to affect current page output. */
class OGMDrupalRender implements OGMRender {
  public function add_meta($name, $content) {
    drupal_set_html_head("<meta property=\"$name\" content=\"{$content}\" />");    
  }
}


/** Interface to getting/setting node tag data. */
interface OGMData {
  public function load_tags($nid);
  public function delete_tags($nid);
  public function update_tags($field_data_including_nid, $primary_key = array());
}
class OGMDrupalData implements OGMData {
  public function load_tags($nid) {
    return db_fetch_object(db_query("SELECT * FROM {".OPENGRAPH_META_TABLE."} WHERE nid = %d", $nid));
  }
  public function delete_tags($nid) {
    db_query("DELETE FROM {".OPENGRAPH_META_TABLE."} WHERE nid = %d", $nid);
  }
  public function update_tags($field_data_including_nid, $primary_key = array()) {
    drupal_write_record(OPENGRAPH_META_TABLE, $field_data_including_nid, $primary_key);
  }
}
