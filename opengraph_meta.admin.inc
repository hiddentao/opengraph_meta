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
    '#description' => t('These can be overridden on a per-node basis.'),
    '#tree' => TRUE,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  foreach ($types as $id => $data) {
    $form['defaults']['types'][OPENGRAPH_META_VAR_CONTENT_TYPE_.$id] = array(
      '#title' => $data->name,
      '#type' => 'select',
      '#options' => OpenGraphMeta::instance()->get_all_og_types_for_select_field(),
      '#description' => t('The value for the %ogtype meta tag for nodes of type %type. If left unset then the %ogtype tag will not get output for this content type.',array('%ogtype' => 'og:type', '%type' => $id)),
      '#default_value' => variable_get(OPENGRAPH_META_VAR_CONTENT_TYPE_.$id, ''),
    );
  }

  // optional tags defaults
  $optionals = new stdClass();
  $optionals->opengraph_meta = variable_get(OPENGRAPH_META_VAR_OPTIONAL_TAGS, array());

  // Location stuff
  $form['defaults']['location'] = array(
    '#type' => 'fieldset',
    '#title' => t('Location tags'),
    '#description' => t('These can be overridden on a per-node basis.'),
    '#tree' => TRUE,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 100,
  );
  $form['defaults']['location'] = array_merge($form['defaults']['location'], _opengraph_meta_location_form_fields($optionals));

  // Contact stuff
  $form['defaults']['contact'] = array(
    '#type' => 'fieldset',
    '#title' => t('Contact tags'),
    '#description' => t('These can be overridden on a per-node basis.'),
    '#tree' => TRUE,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 100,
  );
  $form['defaults']['contact'] = array_merge($form['defaults']['contact'], _opengraph_meta_contact_form_fields($optionals));

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save',
  );

  return $form;
}


function opengraph_meta_settings_form_submit($form, &$form_state) {

  $values = $form_state['values'];

  // save content types enabled
  variable_set(OPENGRAPH_META_VAR_CONTENT_TYPES_ENABLED, $values[OPENGRAPH_META_VAR_CONTENT_TYPES_ENABLED]);

  // save og:type for each content type
  $types = node_get_types();
  foreach ($types as $id => $data) {
    variable_set(OPENGRAPH_META_VAR_CONTENT_TYPE_.$id, $values['types'][OPENGRAPH_META_VAR_CONTENT_TYPE_.$id]);
  }

  // save site name
  variable_set(OPENGRAPH_META_VAR_SITE_NAME, $values[OPENGRAPH_META_VAR_SITE_NAME]);

  // save optional tags
  $options = array();
  foreach (_opengraph_meta_location_form_fields() as $f => $info) {
    $options[$f] = $values['location'][$f];
  }
  foreach (_opengraph_meta_contact_form_fields() as $f => $info) {
    $options[$f] = $values['contact'][$f];
  }
  variable_set(OPENGRAPH_META_VAR_OPTIONAL_TAGS, $options);

  // success
  drupal_set_message("All Open Graph Meta Tags options have been updated");
}