<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');
require_once(dirname(__FILE__) . '/../actions/actions-core.inc');


function configurator_drush_command()
{
    $items = array();
    $items['configure-artifact'] = array(
        'description' => "Configure artifact",
        'arguments' => array(
            'name' => 'the name of artifact',
            'alias' => 'alias for the site the action is applied to. Default is @self which is from current directory. ',
        ),
        'options' => array(
            '--action' => 'list of one or more actions to execute referenced by the index as declared in the blueprint(make file). The action has to be declared in the make file',
            '--tag' => 'list of one or more tags to associated with actions to execute. The action has to be declared in the make file and tag assigned'
        ),
        'examples' => array(
            'ca espresso @workflow --tag=init,kickstart' => 'executes all actions with tag of "init" and then "kickstart" declared in the make file of artifact "espresso"',
            'ca slider @workflow --action=configure_editor,configure_video' => 'configures editor and configures video as declared in blueprint(make file) of artifact - slider for site with alias of @workflow',
            'ca espresso @workflow --tag=dev' => 'execute all actions with tags "dev" as described in the make file for drupal instance with alias "workflow"',
        ),
        'aliases' => array('ca'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
//    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
    );
    return $items;
}

function drush_configure_artifact_init()
{

    read_config();
    read_artifacts();
    init_inputs();
    load_drupal();
    load_workflow();
    init_artifacts();
}

function drush_configurator_configure_artifact($name = 'not-found', $alias = '@self')
{
    drush_log(dt('configuring artifact - @artifact ...', array('@artifact' => $name)), 'ok');

// check if we can bootstrap
    $alias_record = drush_get_context('FLOW_WIDGET_ALIAS');

    if (empty($alias_record)) {
        drush_die("I can't bootstrap from the current location.", 0);
    }

    drush_bootstrap_max_to_sitealias($alias_record);

//let's jump to our site directory before we do anything else
    drush_op('chdir', $alias_record['root']);

    $make_file = get_make_file($name);

    $all_actions = get_actions_from_make($make_file);
    //grab also shared actions from default make file
    $all_actions = $all_actions + get_actions_from_make(drush_get_context('FLOW_CONFIG_DEFAULT_MAKE'));

    $glob_vars = array(); //list of return variables to be reused as specified in make file 'result'
    $glob_tags = array();//list of tags

    $actions = drush_get_context('FLOW_WIDGET_ACTION');
    $active_tags = drush_get_context('FLOW_WIDGET_TAG');

    if (isset($actions) && !empty($actions)) {
        drush_log('about to execute actions - '.implode(',',$actions), 'notice');
        //call solo actions
        foreach($actions as $key => $action_name){
            (isset($all_actions[$action_name])) ? _applay_action($all_actions[$action_name], $glob_vars) : drush_log('action: '.$action_name.' was not found', 'notice');
        }
    } elseif(isset($active_tags) && !empty($active_tags)){
        drush_log('about to execute actions with tags - '.implode(',',$active_tags), 'notice');
        //build tag hash tree
        _build_tags($all_actions, $glob_tags);

        foreach($active_tags as $key => $tag){
            //retrieve all actions for particular tag
            $actions_per_tag = $glob_tags[$tag];

            foreach($actions_per_tag as $key=>$action_name){
                $action = $all_actions[$action_name];
                _applay_action($action, $glob_vars);
            }
        }
    }else{
        drush_log('about to execute all actions: '.$active_tags);
//call each action
        foreach ($active_tags as $key => $action) {
            _applay_action($action, $glob_vars);
        }
    }
    drush_log(dt('done configuring artifact - @artifact', array('@artifact' => $name)), 'ok');
}

function _applay_action($action, &$glob_vars){
    $func = $action['name'] . "_action";
    $params = (isset($action['param']) && !empty($action['param'])) ? _replace_var(array_values($action['param']), $glob_vars) : array();

    $all_commands = drush_get_commands();
    $drush_command = (isset($all_commands[$action['name']])) ? $all_commands[$action['name']] : NULL;


    if (function_exists($func)) {
        //action
        drush_log('begin ACTION - ' . $func . ' with params: '.implode(',',$params),'ok');
        $return = call_user_func_array($func, $params);
        drush_log('end ACTION - ' . $func . ' with params: '.implode(',',$params),'ok');
    } elseif(isset($drush_command) && !empty($drush_command)){
        //drush command
        $alias =  drush_get_context('FLOW_WIDGET_ALIAS');
        drush_log('begin DRUSH COMMAND - ' . $action['name'] . ' with params: '.implode(',',$params),'ok');
        $ret = drush_invoke_process($alias, $action['name'], $params);
        drush_log('end DRUSH COMMAND - ' . $action['name'] . ' with params: '.implode(',',$params),'ok');

        $return = (isset($ret['object'])) ? $ret['object'] : NULL;
    } else {
        drush_log('ERROR. ' . $func . ' does not exist','notice');
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
function _replace_var($params, $g_var){
    $parsed_pars = array();
    foreach($params as $key => $par){
        if($par[0] === '@'){
            $variable_name = substr($par,1);
            $parsed_pars[] = $g_var[$variable_name];
        }else{
            $parsed_pars[] = $par;
        }
    }
    return $parsed_pars;
}

/*
 * builds hash tree from tags
*/
function _build_tags($actions, &$glob_tags){
    //go through each action and grab tags
    foreach($actions as $action_key => $action){
        $tags = explode(',', $action['tag']);
        //store tags in tree
        foreach($tags as $tag_key => $tag){
            if(!isset($glob_tags[$tag]) || !is_array($glob_tags[$tag])){
                $glob_tags[$tag] = array();
            }

            $tag_index = (empty($tag)) ? 'none' : $tag;
            //store key for later use
            $glob_tags[$tag_index][] = $action_key;
        }
    }
}