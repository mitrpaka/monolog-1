<?php

/**
 * @file
 * Administrative settings for the Monolog module.
 */

/**
 * Returns an array of profile options.
 *
 * @return array
 */
function monolog_profile_options() {
  $options = &drupal_static(__FUNCTION__, array());
  if (!$options) {
    $profiles = monolog_profile_load_all();
    foreach ($profiles as $name => $profile) {
      $options[$name] = check_plain($profile->options['label']);
    }
  }
  return $options;
}

/**
 * Channel settings for the Monolog module.
 *
 * @ingroup forms
 */
function monolog_channel_form($form, &$form_state) {
  $channel_info = monolog_channel_info_load_all();
  $channel_profiles = variable_get('monolog_channel_profiles', array());

  $form['description'] = array(
    '#markup' => t('<p>A <strong>channel</strong> identifies which part of the application a record is related to.</p><p>Each channel is associated with a <a href="@href">profile</a> that defines which handlers are used to process the record, for example a <em>syslog handler</em> or <em>stream wrapper handler</em>.</p>', array('@href' => url('admin/config/development/monolog/profile'))),
  );

  $form['channel_table'] = array(
    '#theme' => 'monolog_channel_table',
    '#tree' => TRUE,
    'channel_info' => array(
      '#type' => 'value',
      '#value' => $channel_info,
    ),
    'channels' => array(),
  );

  foreach ($channel_info as $channel_name => $info) {
    if (!isset($channel_profiles[$channel_name])) {
      $channel_profiles[$channel_name] = $info['default profile'];
    }
    $form['channel_table']['channels'][$channel_name]['profile'] = array(
      '#type' => 'select',
      '#options' => monolog_profile_options(),
      '#default_value' => $channel_profiles[$channel_name],
    );
  }

  $form['actions'] = array(
    '#type' => 'actions',
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save channel settings'),
  );

  $form['#submit'] = array('monolog_channel_form_submit');

  return $form;
}

/**
 * Form submission handler for monolog_channel_form().
 */
function monolog_channel_form_submit($form, &$form_state) {
  $channel_profiles = array();
  foreach ($form_state['values']['channel_table']['channels'] as $name => $channel) {
    $channel_profiles[$name] = $channel['profile'];
  }
  variable_set('monolog_channel_profiles', $channel_profiles);
  drupal_set_message(t('Configuation updated.'));
}

/**
 * Returns HTML for the channel table.
 *
 * @param array $variables
 *   An associative array containing:
 *   - element: The FAPI element being themed.
 *
 * @ingroup themeable
 */
function theme_monolog_channel_table(array $variables) {
  $output = '';

  $header = array(
    'channel' => t('Channel'),
    'profile' => t('Profile'),
  );

  $rows = array();
  $channels = $variables['element']['channel_info']['#value'];
  foreach ($channels as $channel_name => $channel_info) {

    // @todo Theme function.
    $label = check_plain($channel_info['label']);
    $machine_name = '<small>' . t('(Machine name: @name)', array('@name' => $channel_name)) . '</small>';
    $descripton = '<div class="description">' . filter_xss_admin($channel_info['description'])  . '</div>';

    $rows[] = array(
      'channel' => $label . ' ' . $machine_name . $descripton,
      'profile' => drupal_render($variables['element']['channels'][$channel_name]['profile']),
    );
  }

  $output .= drupal_render_children($variables['element']);
  $output .= theme('table', array(
    'caption' => t('Logging Channels'),
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'monolog-channel-table'),
  ));

  return $output;
}

/**
 * Page callback; Displayes all profile pages.
 */
function monolog_profile_page() {
  $build = array();

  $build['description'] = array(
    '#markup' =>  t('<p>A <strong>profile</strong> is a collection of handlers that process the record.</p><p>Common examples of handlers are a <em>syslog handler</em> that routes records to the syslog and a <em>stream wrapper handler</em> that writes records to files and other streams.</p>'),
  );

  $header = array(
    'profile' => t('Profile'),
    'handlers' => t('Handlers'),
    'operations' => t('Operations'),
  );

  $rows = array();
  $profiles = monolog_profile_load_all();
  foreach ($profiles as $name => $profile) {
    $base_path = 'admin/config/development/monolog/profile/' . $name;

    // @todo Make a theme function.
    $label = check_plain($profile->options['label']);
    $machine_name = '<small>' . t('(Machine name: @name)', array('@name' => $profile->name)) . '</small>';

    $row = array(
      'profile' => $label . ' ' . $machine_name,
      'handlers' => '',
    );

    $operations = array();
    $operations[] = array(
      'title' => t('Edit'),
      'href' => $base_path . '/edit',
    );
    $operations[] = array(
      'title' => t('Delete'),
      'href' => $base_path . '/delete',
    );

    $row['operations'] = array(
      'data' => array(
        '#theme' => 'links__node_operations',
        '#links' => $operations,
        '#attributes' => array('class' => array('links', 'inline')),
      ),
    );

    $rows[] = $row;
  }

  $build['profiles'] = array(
    '#theme' => 'table',
    '#caption' => t('Logging profiles'),
    '#header' => $header,
    '#rows' => $rows,
  );

  return $build;
}

/**
 * Form for adding and editing logging profile configurations.
 *
 * @param stdClass $profile
 *   The logging profile configuration.
 *
 * @ingroup forms
 */
function monolog_profile_form($form, &$form_state, $profile = null) {
  $form['#is_new'] = (null === $profile);

  if ($form['#is_new']) {
    $profile = monolog_profile_new();
  }

  $form['#monolog'] = array(
    'profile' => $profile,
  );

  $form['handlers'] = array(
    '#type' => 'value',
    '#value' => $profile->options['handlers'],
  );

  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => $profile->options['label'],
    '#description' => t('The human-readable name of the logging profile.'),
    '#required' => TRUE,
    '#maxlength' => 255,
    '#size' => 30,
  );

  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => $profile->name,
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => 'monolog_profile_load',
      'source' => array('label'),
    ),
    '#disabled' => !$form['#is_new'],
    '#description' => t('The machine readable name of the logging profile. This value can only contain letters, numbers, and underscores.'),
  );

  if (!$form['#is_new']) {

    $base_path = 'admin/config/development/monolog/profile';
    $link = l(t('Add handler'), $base_path . '/' . $profile->name . '/handler/add');
    $form['action_link'] = array(
      '#markup' => '<ul class="action-links"><li>' . $link . '</li></ul>',
    );

    $form['channel_table'] = array(
      '#theme' => 'monolog_handler_table',
      '#tree' => TRUE,
      '#monolog' => array(
        'profile' => $profile,
        'handler_info' => monolog_handler_info_load_all(),
      ),
    );

    foreach ($profile->options['handlers'] as $name => $handler) {
      $form['channel_table']['weight'][$name] = array(
        '#type' => 'select',
        '#title' => t('Weight for @handler', array('@handler' => $handler['label'])),
        '#title_display' => 'invisible',
        '#options' => drupal_map_assoc(range(-10, 10)),
        '#default_value' => isset($handler['weight']) ? $handler['weight'] : 0,
        '#attributes' => array('class' => array('monolog-handler-weight')),
      );
    }

    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save profile'),
    );

    $form['actions']['cancel'] = array(
      '#markup' => l(t('Cancel'), $base_path),
    );

    $form['#submit'][] = 'monolog_profile_form_submit';
  }

  return $form;
}

/**
 * Form submission handler for monolog_profile_form().
 */
function monolog_profile_form_submit($form, &$form_state) {
  $profile = $form['#monolog']['profile'];
  form_state_values_clean($form_state);
  $name = $form_state['values']['name'];
  unset($form_state['values']['name']);

  // Add the weights to the handlers if set.
  if (isset($form_state['values']['channel_table']['weight'])) {
    foreach ($form_state['values']['channel_table']['weight'] as $handler_name => $weight) {
      $form_state['values']['handlers'][$handler_name]['weight'] = $weight;
    }
    unset($form_state['values']['channel_table']);
  }

  $profile->name = $name;
  $profile->options = $form_state['values'];

  try {
    if (!monolog_profile_save($profile)) {
      throw new Exception(t('Error saving logging profile configuration.'));
    }

    $form_state['redirect'] = 'admin/config/development/monolog/profile';
    $message = $form['#is_new'] ? t('Configuration saved.') : t('Configuration updated.');
    drupal_set_message($message);

  }
  catch (Exception $e) {
    form_set_error(NULL, $e->getMessage());
    watchdog_exception('monolog', $e);
  }
}

/**
 * Returns HTML for the handler table.
 *
 * @param array $variables
 *   An associative array containing:
 *   - element: The FAPI element being themed.
 *
 * @ingroup themeable
 */
function theme_monolog_handler_table(array $variables) {
  $output = '';

  $header = array(
    t('Label'),
    t('Handler'),
    t('Log Level'),
    t('Weight'),
    t('Operations'),
  );

  $handler_info = $variables['element']['#monolog']['handler_info'];
  $level_options = monolog_level_options();

  $profile = $variables['element']['#monolog']['profile'];

  $rows = array();
  foreach ($profile->options['handlers'] as $name => $handler) {
    $base_path = 'admin/config/development/monolog/profile/' . $profile->name . '/handler/' . $name;

    if (!isset($handler_info[$handler['handler']])) {
      $handler_info[$handler['handler']] = array('label' => $handler['handler']);
    }

    $row = array(
      'label' => check_plain($handler['label']),
      'handler' => check_plain($handler_info[$handler['handler']]['label']),
      'level' => $level_options[$handler['level']],
      'weight' => drupal_render($variables['element']['weight'][$name]),
    );

    $operations = array();
    $operations[] = array(
      'title' => t('Edit'),
      'href' => $base_path . '/edit',
    );
    $operations[] = array(
      'title' => t('Delete'),
      'href' => $base_path . '/delete',
    );

    $row['operations'] = array(
      'data' => array(
        '#theme' => 'links__node_operations',
        '#links' => $operations,
        '#attributes' => array('class' => array('links', 'inline')),
      ),
    );

    $rows[$name] = array(
      'data' => $row,
      'class' => array('draggable'),
    );
  }

  drupal_add_tabledrag('monolog-handler-table', 'order', 'sibling', 'monolog-handler-weight');
  $output .= drupal_render_children($variables['element']);
  $output .= theme('table', array(
    'caption' => t('Logging Handlers'),
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'monolog-handler-table'),
  ));

  return $output;
}

/**
 *
 */
function monolog_profile_import_form($form, &$form_state) {

}

/**
 *
 */
function monolog_profile_export_page($profile) {

}




























/**
 * Page callback; Lists the handlers than can be added.
 */
function monolog_handler_page($profile) {
  $build = array();

  composer_manager_register_autoloader();
  $handler_info = monolog_handler_info_load_all();

  $base_path = 'admin/config/development/monolog/profile/' . $profile->name;

  // @todo Theme function.
  $output = '<dl>';
  foreach ($handler_info as $name => $info) {
    $path = $base_path . '/handler/add/' . $name;
    $output .= '<dt>' . l($info['label'], $path) . '</dt>';
    $output .= '<dd>' . filter_xss_admin($info['description']) . '</dd>';
  }
  $output .= '</dl>';

  $build['handlers'] = array(
    '#markup' => $output,
  );

  return $build;
}

/**
 * Handler settings form.
 *
 * @param stdClass $profile
 *   The logging profile configuration.
 * @param array|null $handler
 *   The configuration options specific to the handler being edited.
 */
function monolog_handler_form($form, &$form_state, stdClass $profile, array $handler = null) {
  $form['#is_new'] = (null === $handler);

  $handler_name = ($form['#is_new']) ? arg(8) : $handler['handler'];
  if (!$handler_info = monolog_handler_info_load($handler_name)) {
    drupal_not_found();
  }

  if ($form['#is_new']) {
    $handler = array(
      'name' => '',
      'label' => '',
      'handler' => $handler_name,
      'options' => $handler_info['default settings'],
    );
  }

  $form['#monolog'] = array(
    'profile' => $profile,
    'handler_name' => $handler_name,
  );

  $form['label'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#default_value' => $handler['label'],
    '#description' => t('The human-readable name of the handler.'),
    '#required' => TRUE,
    '#maxlength' => 255,
    '#size' => 30,
  );

  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => $handler['name'],
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => 'monolog_profile_load',
      'source' => array('label'),
    ),
    '#disabled' => !$form['#is_new'],
    '#description' => t('The machine readable name of the handler. This value can only contain letters, numbers, and underscores.'),
  );

  $form['level'] = array(
    '#title' => 'Logging level',
    '#type' => 'select',
    '#default_value' => $handler['options']['level'],
    '#options' => monolog_level_options(),
    '#description' => t('The minimum severity level of logged messages.'),
  );

  if (isset($handler_info['settings form'])) {
    if (function_exists($handler_info['settings form'])) {
      $handler_info['settings form']($form, $form_state, $profile, $handler);
    }
    else {
      $args = array('@function' => $handler_info['settings form']);
      watchdog('monolog', 'Function not defined: @function', $args, WATCHDOG_ERROR);
    }
  }

  $form['actions'] = array(
    '#type' => 'actions',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save handler'),
  );

  $form['#submit'] = array('monolog_handler_form_submit');

  return $form;
}

/**
 * Form submission handler for monolog_handler_form().
 */
function monolog_handler_form_submit($form, &$form_state) {
  $profile = $form['#monolog']['profile'];
  form_state_values_clean($form_state);
  $name = $form_state['values']['name'];
  unset($form_state['values']['name']);

  $profile->options['handlers'][$name] = $form_state['values'] += array(
    'handler' => $form['#monolog']['handler_name'],
  );

  try {
    if (!monolog_profile_save($profile)) {
      throw new \RuntimeException(t('Error saving handler.'));
    }

    $form_state['redirect'] = 'admin/config/development/monolog/profile/list/' . $profile->name . '/edit';
    $message = $form['#is_new'] ? t('Configuration saved.') : t('Configuration updated.');
    drupal_set_message($message);

  }
  catch (Exception $e) {
    form_set_error(NULL, $e->getMessage());
    watchdog_exception('monolog', $e);
  }
}