<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');
require_once(dirname(__FILE__) . '/../actions/actions-core.inc');


function configurator_drush_command()
{
    $items = array();
    $items['configure-asset'] = array(
        'description' => "Configures asset by applying actions specified by --action or --tag options",
        'arguments' => array(
            'alias' => 'alias for the site the actions are applied to. Default is @self which is from current directory. ',
        ),
        'options' => array(
            'action' => 'list of one or more actions to execute referenced by the index as declared in the blueprint(make file). The action has to be declared in the make file',
//            '--tag' => 'list of one or more tags to associated with actions to execute. The action has to be declared in the make file and tag assigned',
            'tag' => 'list of one or more tags to associated with actions to execute. The action has to be declared in the make file and tag assigned',
            /*different ways to provide blueprint - make file*/
            'package' => 'path to the package of the asset with the make file',
            'ver' => 'version of the new package ',
            'branch' => 'the branch of repo. this can also be package type such as basic, plus, ultimate,etc',
            'asset' => 'name of the asset used to locate make file with actions & artifacts',
            'make' => 'make file that used for configuring the asset'
        ),
        'examples' => array(
            'ca @workflow --tag=config,init,revert --asset=espresso --ver=0.5' => 'executes all actions with tag of "config", then "init" and "revert" as declared in the make file of asset "espresso"',
            'ca @workflow --action=configure_editor,configure_video --asset=slider --ver=0.5' => 'configures editor and configures video as declared in blueprint(make file) of asset - slider for site with alias of @workflow',
            'ca @workflow --tag=dev --asset=espresso' => 'execute all actions with tags "dev" as declared in the make file for drupal instance with alias "workflow"',
            'ca @workflow --tag=editor --make=path/to/some.make' => 'execute all actions with tags "editor" as declared in make file provided by user',
            'ca @workflow --tag=dev --package=path/to/package_dir' => 'execute all actions with tags "editor" as declared in make files stored at package_dir/*',
            'ca @workflow --tag=config,init,dev,revert --package=path/to/package.zip' => 'extracts package into ROOT/.cache/packages, then locates make file from which runs all actions with the tags provided',
            'ca @workflow --tag=config,init,dev,revert --package=path/to/package.tar.gz' => 'extracts package into ROOT/.cache/packages, then locates make file from which runs all actions with the tags provided',
        ),
        'aliases' => array('ca'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
//    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
    );
    return $items;
}

function drush_configure_asset_init()
{
    read_config();
    read_artifacts();
    init_inputs();
    load_drupal();
    load_workflow();
    init_artifacts();
}

function drush_configurator_configure_asset($alias = '@self')
{
    inform_make_supplied();

    $name = wf_var('WF_ASSET_NAME', 'no-asset');
    drush_log(dt('configuring asset - @asset ...', array('@asset' => $name)), 'ok');

// check if we can bootstrap
    $alias_record = drush_get_context('WF_ALIAS');

    if (empty($alias_record)) {
        drush_die("I can't bootstrap from the current location.", 0);
    }

    drush_bootstrap_max_to_sitealias($alias_record);

//let's jump to our site directory before we do anything else
    drush_op('chdir', $alias_record['root']);

    $make_file = get_make_file($name);
    $all_actions = get_actions_from_make($make_file);

    //if package is not set or no make provided, lets import actions from default make file as well
    if (!wf_var('WF_ASSET_PACKAGE') && !wf_var('WF_ASSET_MAKE')) {
        drush_log('adding shared actions from file - ' . drush_get_context('FLOW_CONFIG_DEFAULT_MAKE'), 'notice');
        $all_actions = $all_actions + get_actions_from_make(drush_get_context('WF_DEFAULT_MAKE'));
    }
    drush_log('All actions: '."\n".implode(','."\n",array_keys($all_actions)),'notice');


    $glob_vars = array(); //list of return variables to be reused as specified in make file 'result'
    $glob_tags = array(); //list of tags

    $actions = drush_get_context('WF_INPUT_ACTION');
    $active_tags = drush_get_context('WF_INPUT_TAG');

    if (isset($actions) && !empty($actions)) {
        drush_log('about to execute actions - ' . implode(',', $actions), 'notice');
        //call solo actions
        foreach ($actions as $key => $action_name) {
            (isset($all_actions[$action_name])) ? _applay_action($all_actions[$action_name], $glob_vars) : drush_log('action: ' . $action_name . ' was not found', 'notice');
        }
    } elseif (isset($active_tags) && !empty($active_tags)) {
        drush_log('about to execute actions with tags - ' . implode(',', $active_tags), 'ok');
        //build tag hash tree
        _build_tags($all_actions, $glob_tags);

        foreach ($active_tags as $key => $tag) {
            drush_log('executing tag - ' . $tag, 'ok');
            //retrieve all actions for particular tag
            $actions_per_tag = (isset($glob_tags[$tag])) ? $glob_tags[$tag] : array();
            if (empty($actions_per_tag)) {
                drush_log('No actions found by tag - ' . $tag, 'ok');
            } else {
                foreach ($actions_per_tag as $key => $action_name) {
                    drush_log('executing action - ' . $action_name . ' for tag ' . $tag, 'ok');
                    $action = $all_actions[$action_name];
                    _applay_action($action, $glob_vars);
                }
            }
        }
    } else {
        drush_log('about to execute all actions: ' . $active_tags);
//call each action
        foreach ($active_tags as $key => $action) {
            _applay_action($action, $glob_vars);
        }
    }
    drush_log(dt('done configuring asset - @asset', array('@asset' => $name)), 'ok');
}

function _applay_action($action, &$glob_vars)
{
    $func = $action['name'] . "_action";
    $params = (isset($action['param']) && !empty($action['param'])) ? _replace_var(array_values($action['param']), $glob_vars) : array();

    $all_commands = drush_get_commands();
    $drush_command = (isset($all_commands[$action['name']])) ? $all_commands[$action['name']] : NULL;


    if (function_exists($func)) {
        //action
        drush_log('begin ACTION - ' . $func . ' with params: ' . implode(',', $params), 'ok');
        $return = call_user_func_array($func, $params);
        drush_log('end ACTION - ' . $func . ' with params: ' . implode(',', $params), 'ok');
    } elseif (isset($drush_command) && !empty($drush_command)) {
        //drush command
        $alias = drush_get_context('WF_ALIAS');
        drush_log('begin DRUSH COMMAND - ' . $action['name'] . ' with params: ' . implode(',', $params), 'ok');
        $ret = drush_invoke_process($alias, $action['name'], $params);
        drush_log('end DRUSH COMMAND - ' . $action['name'] . ' with params: ' . implode(',', $params), 'ok');

        $return = (isset($ret['object'])) ? $ret['object'] : NULL;
    } else {
        drush_log('ERROR. ' . $func . ' does not exist', 'notice');
    }

    //keeps global variables for later use
    if (isset($action['result']) && isset($return)) {
        $glob_vars[$action['result']] = $return;
    }
}

/*
 * looks up if any of the parameters is actual global variable
 * @param: $params
 *      parameter list
 * @param: $g_var
 *      list of global variables
 *
 * @returns parsed lists of parameters
 */
function _replace_var($params, $g_var)
{
    $parsed_pars = array();
    foreach ($params as $key => $par) {
        if ($par[0] === '@') {
            $variable_name = substr($par, 1);
            $parsed_pars[] = $g_var[$variable_name];
        } else {
            $parsed_pars[] = $par;
        }
    }
    return $parsed_pars;
}

/*
 * builds hash tree from tags
*/
function _build_tags($actions, &$glob_tags)
{
    //go through each action and grab tags
    foreach ($actions as $action_key => $action) {
        $tags = explode(',', $action['tag']);
        //store tags in tree
        foreach ($tags as $tag_key => $tag) {
            if (!isset($glob_tags[$tag]) || !is_array($glob_tags[$tag])) {
                $glob_tags[$tag] = array();
            }

            $tag_index = (empty($tag)) ? 'none' : $tag;
            //store key for later use
            $glob_tags[$tag_index][] = $action_key;
        }
    }
}