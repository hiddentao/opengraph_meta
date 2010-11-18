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

  $form[OPENGRAPH_META_VAR_CONTENT_TYPES_ENABLED] = array(
    '#title' => t('Enable for the following content types'),
    '#type' => 'checkboxes',
    '#options' => $type_options,
    '#description' => t('The content types for which to enable Open Graph meta tags. If none are selected then tags will be enabled for ALL content types.'),
    '#default_value' => variable_get(OPENGRAPH_META_VAR_CONTENT_TYPES_ENABLED, array()),
  );

  $form['defaults'] = array(
    '#type' => 'fieldset',
    '#title' => 'Default values for meta tags',
  );
  $form['defaults'][OPENGRAPH_META_VAR_SITE_NAME] = array(
    '#title' => t('Site name'),
    '#type' => 'textfield',
    '#description' => t('The value for the %site_name meta tag.',array('%site_name' => 'og:site_name')),
    '#default_value' => variable_get(OPENGRAPH_META_VAR_SITE_NAME, variable_get('site_name','Drupal')),
  );
  $form['defaults']['types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Mapping content type to meta type'),
    '#description' => t('These mappings can be overridden on a node-by-node basis.')
  );

  foreach ($types as $id => $data) {
    $form['defaults']['types'][OPENGRAPH_META_VAR_CONTENT_TYPE_.$id] = array(
      '#title' => $data->name,
      '#type' => 'select',
      '#options' => OpenGraphMeta::get_all_og_types_for_select_field(),
      '#description' => t('The value for the %ogtype meta tag for nodes of type %type. If left unset then the %ogtype tag will not get output for this content type.',array('%ogtype' => 'og:type', '%type' => $id)),
      '#default_value' => variable_get(OPENGRAPH_META_VAR_CONTENT_TYPE_.$id, ''),
    );
  }

  return system_settings_form($form);
}


