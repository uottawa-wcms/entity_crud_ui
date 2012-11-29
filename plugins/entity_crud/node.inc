<?php

$plugin = array(
  'label' => 'Node',
  'handler' => array(
    'class' => 'EntityCRUDUINode',
  ),
);

class EntityCRUDUINode extends EntityCRUDUIBase {

  public function __construct() {
    parent::__construct('node');
  }

  public function customAccess($operation, $entity = NULL) {
    // Revision access is not supported by entity_access, so we need to
    // implement it ourselves.
    if (strlen($operation) > 8) {
      if (substr($operation, 0, 8) == 'revision') {
        $operation = trim(substr($operation, 9));
        return _node_revision_access($entity, $operation);
      }
    }
  }

  public function initializeEntity(&$entity) {
    global $user;
    $entity->title = '';
    $entity->changed = 0;
    $entity->date = 0;
    $entity->created = 0;
    $entity->revision = 0;
    $entity->log = '';
    $entity->name = $user->name;
  }

  public function getPropertyFields($entity, $langcode) {
    $form = array(
      'title' => array(
        '#title' => t('Title'),
        '#type' => 'textfield',
        '#required' => TRUE,
        '#maxlength' => 255,
        '#weight' => -100,
        '#default_value' => $entity->title,
      ),
      'additional_settings' => array(
        '#type' => 'vertical_tabs',
        '#weight' => 5000,
        '#title' => t('Settings'),
        'revision_info' => array(
          '#title' => t('Revision information'),
          '#type' => 'fieldset',
          'revision' => array(
            '#type' => 'checkbox',
            '#title' => t('Create new revision'),
            '#default_value' => 0,
            '#access' => user_access('administer nodes'),
          ),
          'log' => array(
            '#type' => 'textarea',
            '#title' => t('Revision log message'),
            '#rows' => 4,
            '#default_value' => '',
            '#description' => t('Provide an explanation of the changes you are making. This will help other authors understand your motivations.'),
          ),
        ),
        'author' => array(
          '#title' => t('Authoring information'),
          '#type' => 'fieldset',
          '#access' => user_access('administer nodes'),
          'name' => array(
            '#type' => 'textfield',
            '#title' => t('Authored by'),
            '#maxlength' => 60,
            '#autocomplete_path' => 'user/autocomplete',
            '#default_value' => $entity->name,
            '#weight' => -1,
            '#description' => t('Leave blank for %anon', array(
              '%anon' => variable_get('anonymous', t('Anonymous')),
            )),
          ),
          'created' => array(
            '#type' => 'textfield',
            '#title' => t('Authored on'),
            '#maxlength' => 25,
            '#description' => t('Format: %time. The date format is YYYY-MM-DD and %timezone is the time zone offset from UTC. Leave blank to use the time of form submission.', array('%time' => !empty($entity->date) ? date_format(date_create($entity->date), 'Y-m-d H:i:s O') : format_date($entity->created, 'custom', 'Y-m-d H:i:s O'), '%timezone' => !empty($entity->date) ? date_format(date_create($entity->date), 'O') : format_date($entity->created, 'custom', 'O'))),
            '#default_value' => !empty($entity->date) ? $entity->date : '',
          ),
        ),
        'publishing' => array(
          '#title' => t('Publishing information'),
          '#type' => 'fieldset',
          '#access' => user_access('administer nodes'),
          'status' => array(
              '#type' => 'checkbox',
              '#title' => t('Published'),
              '#default_value' => $entity->status,
            ),
          'promote' => array(
              '#type' => 'checkbox',
              '#title' => t('Promoted to front page'),
              '#default_value' => $entity->promote,
            ),
          'sticky' => array(
              '#type' => 'checkbox',
              '#title' => t('Sticky at top of lists'),
              '#default_value' => $entity->sticky,
            ),
        ),
      ),
    );
    return $form;
  }

  public function propertyAttach(&$entity, $form, &$form_state) {
    $loadProperties = array(
      'title', 'status', 'promote', 'sticky', 'changed',
    );
    foreach ($loadProperties as $propkey) {
      if (!empty($form_state['values'][$propkey])) {
        $entity->$propkey = $form_state['values'][$propkey];
      }
    }
    if (!empty($form_state['values']['revision'])) {
      $entity->revision = TRUE;
      if (!empty($form_state['values']['log'])) {
        $entity->log = $form_state['values']['log'];
      }
    }
    $user = user_load_by_name($form_state['values']['name']);
    if (!empty($user)) {
      $entity->name = $user->name;
      $entity->uid = $user->uid;
    } else {
      $entity->name = '';
      $entity->uid = 0;
    }
    parent::propertyAttach($entity, $form, $form_state);
  }

  public function presave(&$entity) {
    if (empty($entity->changed)) {
      $entity->changed = time();
    }
    if (empty($entity->created)) {
      $entity->created = time();
    }
    if (empty($entity->language)) {
      $entity->language = 'und';
    }
  }
}