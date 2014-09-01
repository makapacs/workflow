<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');
require_once(dirname(__FILE__) . '/../actions/actions-core.inc');


function executer_drush_command()
{
    $items = array();
    $items['execute-action'] = array(
        'description' => "Execute Action",
        'arguments' => array(
            'name' => 'name of the action',
            'alias' => 'alias for the site the action is applied to. Default is @self which is from current directory. The action has to be declared in the make file or ...[@ToDO]',
        ),
        'options' => array(
            '--param' => 'list of arguments to pass action if called dynamically',
        ),
        'examples' => array(
            'ea configure_editor @workflow --param=designssquare_com_ckeditor,ckeditor' => 'configures editor for site with alias of @workflow called dynamically, meaning, without action declared beforehand',
            'ea core-status @workflow --param=version' => 'action can also be any drush command. Here, drush command core-status is executed as an action on alias instance "workflow"',
            'ea install-site --param=@workflow' => 'action can also be any drush command without alias specified. Here, drush command install-site is executed as an action"',
        ),
        'aliases' => array('ea'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    );
    return $items;
}

function drush_execute_action_init()
{
    load_workflow();
    load_slim_drupal();
    read_config();
//    read_artifacts();
    init_inputs();
//    init_artifacts();
}

function drush_executer_execute_action($name = 'not-found', $alias = '@self')
{
    drush_log(dt('executing action - @action ...', array('@action' => $name)), 'ok');

// check if we can bootstrap
    $alias_record = drush_get_context('FLOW_WIDGET_ALIAS');
    if (empty($alias_record)) {
        drush_die("I can't bootstrap from the current location.", 0);
    }

//let's jump to our site directory before we do anything else
    drush_op('chdir', $alias_record['root']);


//param
    $action = array(
        'name' => $name,
        'param' => drush_get_context('FLOW_WIDGET_PARAM',array()),
    );

    $ret = apply_action($action);
    if(isset($ret['output']) && !empty($ret['output'])){
        drush_print($ret['output']);
    }
    drush_log(dt('done executing action - @action', array('@action' => $name)), 'ok');
}