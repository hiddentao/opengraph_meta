<?php
// $Id$

require_once('opengraph_meta.common.inc');


/**
 * Menu callback: settings form.
 */
function opengraph_meta_settings_form() {

  $types = node_get_types();

  $type_options = array();
  foreach ($types as $id => $data) {
    $type_options[$id] = $data->name;
  }

  $form[OPENGRAPH_META_VAR_CONTENT_TYPES] = array(
    '#title' => t('Content types'),
    '#type' => 'checkboxes',
    '#options' => $type_options,
    '#description' => t('The content types for which to enable Open Graph meta tags. If none are selected then it will be enabled for ALL content types.'),
    '#default_value' => variable_get(OPENGRAPH_META_VAR_CONTENT_TYPES, array()),
  );

  return system_settings_form($form);
}
