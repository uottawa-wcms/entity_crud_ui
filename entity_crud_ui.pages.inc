<?php

function entity_crud_ui_create_entity_list($entity_type) {
  $bundles = $entity_type->getBundles();
  if (count($bundles) == 1) {
    drupal_goto('entity/' . $entity_type->getType() . '/' . reset(array_keys($bundles)) . '/add');
    exit();
  }
  $content = array();
  foreach ($bundles as $key => $bundle) {
    $content[] = array(
      'title' => $bundle['label'],
      'href' => 'entity/' . $entity_type->getType() . '/' . $key . '/add',
      'description' => !empty($type['description']) ? $type['description'] : '',
      'localized_options' => array(),
    );
  }
  return theme('admin_block_content', array('content' => $content));
}

function entity_crud_ui_create_entity($entity_type, $bundle_type) {
  $entity = $entity_type->newObject($bundle_type);
  return drupal_get_form('entity_crud_ui_entity_form', $entity_type, $entity);
}

function entity_crud_ui_entity_form($form, &$form_state, $entity_type, $entity) {
  $entity_type->prepareObject($entity);
  global $language;
  $lang = $language->language;
  if (isset($form_state['values']['language'])) {
    $lang = $form_state['values']['language'];
  }
  $langs = array();
  $languages = language_list();
  foreach ($languages as $key => $language) {
    $langs[$key] = t($language->name);
  }
  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $entity_type,
  );
  $form['entity'] = array(
    '#type' => 'value',
    '#value' => $entity,
  );
  $form['language'] = array(
    '#type' => 'select',
    '#title' => t('Current Language'),
    '#options' => $langs,
    '#default_value' => $lang,
    '#ajax' => array(
      'callback' => 'entity_crud_ui_reload_language',
      'wrapper' => 'field-container',
      'effect' => 'fade',
    ),
  );
  $form['language-dependent'] = array(
    '#type' => 'container',
    '#prefix' => '<div id="field-container">',
    '#suffix' => '</div>',
  );
  $entity_type->alterForm($form['language-dependent'], $form_state, $entity, $lang);
  field_attach_form($entity_type->getType(), $entity, $form['language-dependent'], $form_state, $lang);
  $form['actions'] = array(
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Save'),
      '#weight' => -100,
    ),
  ) + $entity_type->getFormActions();
  if (empty($entity->is_new)) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('entity_crud_ui_delete_redirect'),
      '#weight' => 100,
    );
  }
  return $form;
}

function entity_crud_ui_delete_redirect($form, &$form_state) {
  $form_state['redirect'] = $form_state['values']['entity_type']->uri(
      $form_state['values']['entity'],
      'delete'
  );
}

function entity_crud_ui_entity_form_validate($form, &$form_state) {
  $entity_type = $form_state['values']['entity_type'];
  $entity_type->formValidate(
      $form_state['values']['entity'],
      $form,
      $form_state
  );
  field_attach_form_validate(
      $entity_type->getType(),
      $form_state['values']['entity'],
      $form,
      $form_state
  );
}

function entity_crud_ui_entity_form_submit($form, &$form_state) {
  $entity_type = $form_state['values']['entity_type'];
  $entity = $form_state['values']['entity'];
  entity_form_submit_build_entity($entity_type->getType(), $entity, $form, $form_state);
  $entity_type->propertyAttach($entity, $form, $form_state);
  $entity_type->presave($entity);
  $entity_type->save($entity);
  $entity_type->postsave($entity, $form_state);
}

function entity_crud_ui_view_entity($entity_type, $entity) {
  drupal_set_title($entity_type->label($entity));
  return $entity_type->view($entity);
}

function entity_crud_ui_entity_delete_form($form, &$form_state, $entity_type, $entity) {
  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $entity_type,
  );
  $form['entity'] = array(
    '#type' => 'value',
    '#value' => $entity,
  );
  $form = confirm_form($form,
      t('Are you sure you want to delete this entity?'),
      $entity_type->uri($entity)
  );
  return $form;
}

function entity_crud_ui_entity_delete_form_submit($form, &$form_state) {
  $type = $form_state['values']['entity_type'];
  $type->delete($form_state['values']['entity'], $form_state);
  drupal_set_message(t('Entity deleted successfully'));
  $form_state['redirect'] = 'entity/' . $form_state['values']['entity']->type;
}

function entity_crud_ui_entity_revision_form($form, &$form_state, $entity_type, $entity) {
  $revisions = $entity_type->revisions($entity);
  if (empty($revisions)) {
    drupal_set_message(t('This entity type does not support revisions'), 'warning');
    return $form;
  }
  if ($revisions->rowCount() < 2) {
    drupal_set_message(t('There is only one revision for this entity'), 'warning');
    return $form;
  }

  $form['#tree'] = TRUE;
  $form['#theme'] = 'entity_crud_ui_revisions';

  $form['entity'] = array(
    '#type' => 'value',
    '#value' => $entity,
  );
  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $entity_type,
  );

  $revert = entity_crud_ui_access('revert revisions', $entity_type, $entity);
  $delete = entity_crud_ui_access('delete revisions', $entity_type, $entity);

  $form['#headers'] = $entity_type->getRevisionHeaders();
  foreach (array_keys($form['#headers']) as $key) {
    $form[$key] = array();
  }
  $form['operations'] = array();

  $first = TRUE;
  $keys = array();
  $first_key = $second_key = NULL;
  foreach ($revisions as $revision) {
    $rev_key = $revision->revision_id;
    if (empty($first_key)) { $first_key = $rev_key; }
    elseif (empty($second_key)) { $second_key = $rev_key; }
    $info = $entity_type->getRevisionData($entity, $revision);
    $keys[$rev_key] = '';
    foreach ($info as $key => $data) {
      $form[$key][$rev_key] = array(
        '#markup' => $data,
      );
    }
    $ops = array();
    $ops[] = l(t('view'), $entity_type->revisionURI($entity, $revision, 'view'));
    if ($revert && (!$first)) {
      $ops[] = l(t('revert'), $entity_type->revisionURI($entity, $revision, 'revert'));
    }
    if ($delete && (!$first)) {
      $ops[] = l(t('delete'), $entity_type->revisionURI($entity, $revision, 'delete'));
    }
    $form['operations'][$rev_key] = array(
      '#markup' => implode(' | ', $ops),
    );
    $first = FALSE;
  }
  $form['#compare'] = FALSE;
  if (module_exists('diff')) {
    $form['#compare'] = TRUE;
    $form['old'] = array(
      '#type' => 'radios',
      '#default_value' => $second_key,
      '#options' => $keys,
    );
    $form['new'] = array(
      '#type' => 'radios',
      '#default_value' => $first_key,
      '#options' => $keys,
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Compare'),
    );
  }

  return $form;
}

function entity_crud_ui_view_revision_diff($entity_type, $entity, $revision1, $revision2, $state = NULL) {
  drupal_set_title(t('Revisions for @title', array(
    '@title' => $entity_type->label($entity),
  )));
  $table = $entity_type->generateDiffView($entity, $revision1, $revision2, $state);
  return $table;
}

function entity_crud_ui_entity_revision_form_validate($form, &$form_state) {
  if (empty($form_state['values']['old'])) {
    form_set_error('old', t('You must pick a left-hand revision'));
  }
  if (empty($form_state['values']['new'])) {
    form_set_error('new', t('You must pick a right-hand revision'));
  }
  if ($form_state['values']['old'] == $form_state['values']['new']) {
    form_set_error('new', t('Comparing the same revision is kind of silly, don\'t you think?'));
  }
}

function entity_crud_ui_entity_revision_form_submit($form, &$form_state) {
  $form_state['redirect'] = $form_state['values']['entity_type']->getDiffCompareURI(
      $form_state['values']['entity'],
      $form_state['values']['old'],
      $form_state['values']['new']
  );
}

function entity_crud_ui_view_revision($entity_type, $entity, $revision) {
  $ventity = $entity_type->entityByVID($revision);
  return $entity_type->view($ventity);
}

function entity_crud_ui_revert_revision_form($form, &$form_state, $entity_type, $entity, $revision) {
  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $entity_type,
  );
  $form['entity'] = array(
    '#type' => 'value',
    '#value' => $entity,
  );
  $form['revision'] = array(
    '#type' => 'value',
    '#value' => $revision,
  );
  return confirm_form($form,
      t('Are you sure you want to revert to this older version?'),
      $entity_type->uri($entity, 'revisions')
  );
}

function entity_crud_ui_revert_revision_form_submit($form, &$form_state) {
  $form_state['values']['entity_type']->revert(
      $form_state['values']['entity'],
      $form_state['values']['revision']
  );
  drupal_set_message(t('Entity reverted successfully'));
  $form_state['redirect'] = $form_state['values']['entity_type']->uri(
      $form_state['values']['entity']
  );
}

function entity_crud_ui_delete_revision_form($form, &$form_state, $entity_type, $entity, $revision) {
  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $entity_type,
  );
  $form['entity'] = array(
    '#type' => 'value',
    '#value' => $entity,
  );
  $form['revision'] = array(
    '#type' => 'value',
    '#value' => $revision,
  );
  return confirm_form($form,
      t('Are you sure you want to delete this older version?'),
      $entity_type->uri($entity, 'revisions')
  );
}

function entity_crud_ui_delete_revision_form_submit($form, &$form_state) {
  $result = $form_state['values']['entity_type']->deleteRevision(
      $form_state['values']['entity'],
      $form_state['values']['revision']
  );
  if ($result) {
    drupal_set_message(t('Revision deleted successfully'));
  } else {
    drupal_set_message(t('Could not delete revision'), 'error');
  }
  $form_state['redirect'] = $form_state['values']['entity_type']->uri(
      $form_state['values']['entity']
  );
}

