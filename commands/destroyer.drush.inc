<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');


function destroyer_drush_command()
{
    $items = array();
    $items['destroy-it'] = array(
        'description' => "removes an instance including dropping database, removing root, etc",
        'arguments' => array(
            '@alias' => 'alias name of the instance to apply actions. Make sure to include "@" in front',
//            'makefile' => 'blueprint of drupal installation'
        ),
        'options' => array(),
        'examples' => array(
            'drush di @workflow' => 'drops tables and removes dir of alias - workflow',
        ),
        'aliases' => array('di'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
//        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_DATABASE
    );
    return $items;
}

function drush_destroy_it_init()
{
    load_workflow();
    read_config();
//    init_workflow();
}

function drush_destroyer_destroy_it($alias = 'self')
{
    drush_log('destroying drupal instance...', 'ok');
    $default_dest = '';

    //retrieve site alias
    $site_record = drush_sitealias_get_record($alias);
    // check if we can bootstrap
    if (empty($site_record)) {
        drush_die("I can't bootstrap from the current location.", 0);
    }

    //Retrieve DB settings
    $result_record = drush_invoke_process($site_record, 'core-status', array());
    $stats = $result_record['object'];

    drush_log(dt("dropping tables for database ...@credentials", array('@credentials' => implode(',', $result_record))), 'ok');
    $status = drush_invoke_process('@self', 'sql-drop', array(), array('db-url' => 'mysql://' . $stats['db-username'] . ':' . $stats['db-password'] . '@' . $stats['db-hostname'] . ':' . $stats['db-port'] . '/' . $result_record['db-name'], 'yes' => TRUE));
    drush_log((($status) ? 'Tables were dropped' : 'FAILED to drop tables'), 'ok');

    drush_log(dt('removing dir - @dir.....', array('@dir' => $site_record['root'])), 'ok');
    execute_command('sudo rm -R ' . $site_record['root']);

    drush_log(dt('Done Destroying instance @location', array('@location' => $default_dest)), 'ok');
}