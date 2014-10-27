<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');


function destroyer_drush_command()
{
    $items = array();
    $items['blow-it'] = array(
        'description' => "removes an instance including dropping database, removing root, etc",
        'arguments' => array(
            '@alias' => 'alias name of the instance to apply actions. Make sure to include "@" in front',
//            'makefile' => 'blueprint of drupal installation'
        ),
        'options' => array(),
        'examples' => array(
            'drush bi @workflow' => 'drops tables and removes dir of alias - workflow',
        ),
        'aliases' => array('bi'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
//        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_DATABASE
    );
    return $items;
}

function drush_blow_it_init()
{

    read_config();
    init_inputs();
    load_workflow();
}

function drush_destroyer_blow_it($alias = '@self')
{

    //retrieve site alias
    $site_record = wf_var('WF_ALIAS',FALSE);
    // check if we can bootstrap
    if (empty($site_record)) {
        drush_die("I can't bootstrap from the current location.", 0);
    }
    drush_log('destroying drupal instance at ' . $site_record['root'] . '...', 'ok');

    //Retrieve DB settings
    $result_record = drush_invoke_process($site_record, 'core-status', array());
    $stats = (isset($result_record['object'])) ? $result_record['object'] : drush_die('ERROR: unable to retrieve db settings...',0);
    drush_log(dt("dropping tables for database ..."), 'ok');
    $status = drush_invoke_process($site_record, 'sql-drop', array(), array('db-url' => 'mysql://' . $stats['db-username'] . ':' . $stats['db-password'] . '@' . $stats['db-hostname'] . ':' . $stats['db-port'] . '/' . $result_record['db-name'], 'yes' => TRUE));
    drush_log((($status) ? 'Tables were dropped' : 'FAILED to drop tables'), 'ok');

    //removing dir
    drush_log(dt('removing dir - @dir.....', array('@dir' => $site_record['root'])), 'ok');
    execute_command('sudo rm -R ' . $site_record['root']);

    drush_log(dt('Done Destroying instance at @location', array('@location' => $site_record['root'])), 'ok');
}