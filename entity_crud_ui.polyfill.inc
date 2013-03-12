<?php

if (module_exists('title')) {
  if (!function_exists('title_entity_crud_ui_node_properties_alter')) {
    function title_entity_crud_ui_node_properties_alter($entity, &$properties) {
      $replacements = title_field_replacement_info('node');
      if ($replacements) {
        foreach ($replacements as $field_name => $field) {
          if (isset($properties[$field_name]) && title_field_replacement_enabled('node', $entity->type, $field_name)) {
            $properties[$field_name]['#access'] = FALSE;
            $properties[$field_name]['#field_replacement'] = TRUE;
          }
        }
      }
    }
  }
  if (!function_exists('title_entity_crud_ui_node_properties_attach')) {
    function title_entity_crud_ui_node_properties_attach($form, &$form_state, &$entity) {
      title_entity_sync($entity->entity_type->getType(), $entity);
    }
  }
}

if (module_exists('comment')) {
  if (!function_exists('comment_entity_crud_ui_node_properties_attach')) {
    function comment_entity_crud_ui_node_properties_attach($form, &$form_state, &$entity) {
      $entity->comment = $form_state['values']['comment'];
    }
  }

  if (!function_exists('comment_entity_crud_ui_node_properties_alter')) {
    function comment_entity_crud_ui_node_properties_alter($entity, &$properties) {
      $comment_count = isset($entity->nid) ? db_query('SELECT comment_count FROM {node_comment_statistics} WHERE nid = :nid', array(':nid' => $entity->nid))->fetchField() : 0;
      $comment_settings = ($entity->comment == COMMENT_NODE_HIDDEN && empty($comment_count)) ? COMMENT_NODE_CLOSED : $entity->comment;
      $properties['additional_settings']['comment_settings'] = array(
        '#type' => 'fieldset',
        '#access' => user_access('administer comments'),
        '#title' => t('Comment settings'),
        '#weight' => 30,
        'comment' => array(
          '#type' => 'radios',
          '#title' => t('Comments'),
          '#title_display' => 'invisible',
          '#default_value' => $comment_settings,
          '#options' => array(
            COMMENT_NODE_OPEN => t('Open'),
            COMMENT_NODE_CLOSED => t('Closed'),
            COMMENT_NODE_HIDDEN => t('Hidden'),
          ),
          COMMENT_NODE_OPEN => array(
            '#description' => t('Users with the "Post comments" permission can post comments.'),
          ),
          COMMENT_NODE_CLOSED => array(
            '#description' => t('Users cannot post comments, but existing comments will be displayed.'),
          ),
          COMMENT_NODE_HIDDEN => array(
            '#description' => t('Comments are hidden from view.'),
          ),
        ),
      );
      if (empty($comment_count)) {
        $properties['additional_settings']['comment_settings']['comment'][COMMENT_NODE_HIDDEN]['#access'] = FALSE;
        $properties['additional_settings']['comment_settings']['comment'][COMMENT_NODE_CLOSED]['#description'] = t('Users cannot post comments.');
      }
    }
  }
}

if (module_exists('menu')) {

  if (!function_exists('menu_entity_crud_ui_node_properties_attach')) {
    function menu_entity_crud_ui_node_properties_attach($form, &$form_state, &$entity) {
      $entity->menu['enabled'] = $form_state['values']['menu']['enabled'];
      if (!empty($form_state['values']['menu']['link']['parent'])) {
        $pieces = explode(":", $form_state['values']['menu']['link']['parent']);
        $entity->menu['menu_name'] = $pieces[0];
        $entity->menu['plid'] = $pieces[1];
        $entity->menu = $form_state['values']['menu']['link'] + $entity->menu;
      }
    }
  }

  if (!function_exists('menu_entity_crud_ui_node_properties_alter')) {
    function menu_entity_crud_ui_node_properties_alter($entity, &$properties, $langcode = 'und') {
      $default = ($entity->menu['mlid'] ? $entity->menu['menu_name'] . ":" .
          $entity->menu['plid'] : variable_get('menu_parent_' . $entity->type, 'main-menu:0'));
      $options = menu_parent_options(
          menu_get_menus(),
          $entity->menu['mlid'] ? $entity->menu : $entity->type,
          $entity->type);
      if (!isset($options[$default])) {
        $default = reset(array_keys($options));
      }
      $properties['additional_settings']['menu'] = array(
        '#type' => 'fieldset',
        '#title' => t('Menu settings'),
        '#weight' => -2,
        '#access' => user_access('administer menu'),
        '#tree' => TRUE,
        'enabled' => array(
          '#type' => 'checkbox',
          '#title' => t('Provide a menu link'),
          '#default_value' => (int) (bool) $entity->menu['mlid'],
        ),
        'link' => array(
          '#type' => 'container',
          '#states' => array(
            'invisible' => array(
              'input[name="menu[enabled]"]' => array('checked' => FALSE),
            )
          ),
          'link_title' => array(
            '#type' => 'textfield',
            '#title' => t('Menu link title'),
            '#default_value' => $entity->menu['link_title'],
            '#description' => t('You will be able to translate menu items using string translation.'),
          ),
          'description' => array(
            '#type' => 'textarea',
            '#title' => t('Description'),
            '#default_value' => isset($entity->menu['options']['attributes']['title']) ? $entity->menu['options']['attributes']['title'] : '',
            '#rows' => 1,
            '#description' => t('Shown when hovering over the menu link.'),
          ),
          'parent' => array(
            '#type' => 'select',
            '#title' => t('Parent item'),
            '#default_value' => $default,
            '#options' => $options,
          ),
          'weight' => array(
            '#type' => 'weight',
            '#title' => t('Weight'),
            '#delta' => 50,
            '#default_value' => $entity->menu['weight'],
            '#description' => t('Menu links with smaller weights are displayed before links with larger weights.'),
          ),
        ),
      );
      foreach (array('mlid', 'module', 'hidden', 'has_children', 'customized', 'options', 'expanded', 'hidden', 'parent_depth_limit') as $key) {
        $properties['additional settings']['menu']['link'][$key] = array(
          '#type' => 'value',
          '#value' => $entity->menu[$key],
        );
      }
    }
  }
}