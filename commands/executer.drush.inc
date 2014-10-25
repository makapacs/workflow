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
        ),
        'options' => array(
            'param' => 'list of arguments to pass action if called dynamically',
            'alias' => 'the alias name(i.e. @some) of the site at which the actions are applied',

            /*different ways to provide blueprint - make file*/
            'asset' => 'the asset to locate make file with actions',
            'package' => 'the package to locate make file with actions',
            'make' => 'the make file with actions',
            'ver' => 'version of the new package ',
            'branch' => 'the branch of repo. this can also be package type such as basic, plus, ultimate,etc',

        ),
        'examples' => array(
            'ea configure_editor --alias=@workflow --param=designssquare_com_ckeditor,ckeditor' => 'configures editor for site with alias of @workflow called dynamically, meaning, without action declared beforehand',
            'ea core-status --alias=@workflow --param=version' => 'action can also be any drush command. Here, drush command core-status is executed as an action on alias instance "workflow"',
            'ea install-site --param=@workflow' => 'action can also be any drush command without alias specified. Here, drush command install-site is executed as an action"',
            'ea list_uncommitted_artifacts --asset=espresso' => 'list all artifacts that have been updated but not committed in the working dir for the particular asset',
            'ea update_all_work_dir --param="test update" --asset=espresso' => 'goes through each artifact and commits/push/increments version/adds tag if there is changes',
            'ea get_artifact_info --asset=espresso' => 'displays and compares each artifact versions between blueprint and working dir',
            'ea create_artifact --param=metronic --verbose' => 'creates boilerplate artifact work directory and adds/commits/pushes first commit',
        ),
        'aliases' => array('ea'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    );
    return $items;
}

function drush_execute_action_init()
{
//    load_workflow();
////    load_slim_drupal();
//    load_drupal();
//    read_config();
//    read_artifacts();
//    init_inputs();
//    //DRUPAL cannot be loaded for general actions
////    load_drupal();
//    init_artifacts();

    read_config();
    read_artifacts();
    init_inputs();
    //@Todo fix implement mechanism to load full drupal when executed solo for actions that need drupal only
        load_slim_drupal();
//    load_drupal();
    load_workflow();
    init_artifacts();
}

function drush_executer_execute_action($name = 'not-found', $alias = '@self')
{
    inform_make_supplied();

    drush_log(dt('..executing action - @action'."\n", array('@action' => $name)), 'ok');

    // check if we can bootstrap
    $alias_record = drush_get_context('WF_ALIAS');
    if (empty($alias_record)) {
        drush_log("executing in general..no alias specified", 'notice');
    }else{
        $initial_dir = getcwd();
        //let's jump to our site directory before we do anything else
        drush_op('chdir', $alias_record['root']);
    }

    //param
    $action = array(
        'name' => $name,
        'param' => drush_get_context('WF_INPUT_PARAM',array()),
    );

    $ret = apply_action($action);
    if(isset($ret['output']) && !empty($ret['output'])){
        drush_print($ret['output']);
    }
    if(isset($initial_dir)){
        //return to initial dir
        drush_op('chdir',$initial_dir);
    }
    drush_log(dt("\n".'...done executing action - @action', array('@action' => $name)), 'ok');
}