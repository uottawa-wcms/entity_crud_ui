<?php

function theme_entity_crud_ui_revisions($vars) {
  $output = '';
  $header = $vars['form']['#headers'];
  if (!empty($vars['form']['#compare'])) {
    $header[] = array(
      'colspan' => 2,
      'data' => drupal_render($vars['form']['submit']),
    );
  }
  $header['operations'] = t('Operations');
  $rows = array();
  $diff_keys = array_keys($vars['form']['old']['#options']);
  foreach ($diff_keys as $revkey) {
    $row = array();
    foreach (array_keys($vars['form']['#headers']) as $header_key) {
      $row[$header_key] = drupal_render($vars['form'][$header_key][$revkey]);
    }
    if (!empty($vars['form']['#compare'])) {
      $row[] = drupal_render($vars['form']['old'][$revkey]);
      $row[] = drupal_render($vars['form']['new'][$revkey]);
    }
    $row['operations'] = drupal_render($vars['form']['operations'][$revkey]);
    $rows[] = $row;
  }
  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
  ));
  $output .= drupal_render_children($vars['form']);
  return $output;
}
