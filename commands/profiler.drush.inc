<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');
require_once(dirname(__FILE__) . '/../actions/actions-core.inc');


function profiler_drush_command()
{
    $items = array();
    $items['profile-asset'] = array(
        'description' => "install, deploy and configure a drupal instance from a package",
        'arguments' => array(
            'profile' => 'name of profile. It is the name of the profile make file in the /profile dir',
            'alias' => 'alias for the site the action is applied to. Default is @self which is from current directory. ',
        ),
        'options' => array(
            'tag' => 'list of one or more tags to associated with actions to execute on the package in addition to "config,init,revert" at the configuration stage',
            'env' => 'the environment such as test, dev, stage used at deployment stage',

            /***db specific**/
            'db-user' => 'database user',
            'db-pwd' => 'database password',
            'db-name' => 'database name',

            /*different ways to provide blueprint - make file*/
            'package' => 'path to the package containing make file used for profiling asset. If "env" is not specified,it will default to TEST deployment',
            'make' => 'path to the make file for profiling asset. If "env" is not specified, it will default to STAGE deployment',
            'asset' => 'asset used to look up the make file for profiling asset. If "env" is not specified, it will default to DEV deployment',
            'ver' => 'ver for the make file of the asset to profile',
            'branch' => 'the branch of repo. this can also be package type such as basic, plus, ultimate,etc',
        ),
        'examples' => array(
            'pa installer-basic @workflow --env=test --pacakge=path/to/package/dir' => 'installs new site, deploys asset from the package and configures per actions with tags config,init,revert',
            'pa new-install @workflow --package=themes/designssquare-com-theme-espresso/designssquare-com-theme-espresso-0.5' => 'installs and deploys from package in test environment',
            'pa new-install @workflow --make=themes/designssquare-com-theme-espresso.make' => 'installs and downloads in stage environment since make file provided',
            'pa new-install @workflow --ver=0.5 --asset=espresso' => 'installs and downloads in dev environment',
        ),
        'aliases' => array('pa'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
//    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
    );
    return $items;
}

function drush_profile_asset_init($profile = 'dependent-install', $alias = '@self')
{
    read_config();
    read_artifacts();
    init_inputs();
    load_slim_drupal();
//    load_drupal();
    load_workflow();
    init_artifacts();
}

function drush_profiler_profile_asset($profile = 'dependent-install', $alias = '@self')
{
    inform_make_supplied();
    drush_log(dt('executing profile  - @profile ...', array('@profile' => $profile)), 'ok');

// check if we can bootstrap
    $alias_record = drush_get_context('WF_ALIAS');
//    $active_tags = drush_get_context('WF_INPUT_TAG');

    if (empty($alias_record)) {
        drush_die("I can't bootstrap from the current location.", 0);
    }

    $profile_blueprint = dirname(__FILE__) . '/../profiles/'.drush_get_context('WF_PROFILE_NAME').'.make';

    if(!file_exists($profile_blueprint)){
        return drush_set_error('WORKFLOW_PROFILE_ERROR', dt('ERROR: profile blueprint - '.$profile_blueprint.' doesn\'t exist' ));
    }

    $all_actions = get_actions_from_make($profile_blueprint);
    //grab actions from package make file
//    $package_dir = drush_get_context('WF_ASSET_PACKAGE');
//    $package_make = get_actions_from_dir($package_dir);

//    $all_actions = $all_actions + get_actions_from_make(drush_get_context('WF_ASSET_PACKAGE'));

    foreach($all_actions as $action){
        //replace variable to values for parameters

        $params = (isset($action['param']) && !empty($action['param'])) ? _parse_param($action['param']) : array();
        $options = (isset($action['flag']) && !empty($action['flag'])) ? _parse_param($action['flag']) : array();
        $target_action = array(
            'name' => $action['name'],
            'param' => $params,
            'option' => $options,
        );

        drush_log('target action: '.implode(','."\n",$target_action),'notice');
        _prepare_deploy_action($target_action);
        _prepare_config_asset_action($target_action);
        print_r($target_action );
        $ret = apply_action($target_action);
        if(isset($ret['output']) && !empty($ret['output'])){
            drush_print($ret['output']);
        }
    }

    drush_log(dt('done executing profile  - @profile', array('@profile' => $name)), 'ok');
}

/*
 * parses each variable into value
 * @param: $params
 *      array of parameters with variables
 * @return: parsed parameters with values
 */
function _parse_param($params){
    $parsed_par = array();
    $temp_par = '';
    foreach($params as $key => $param){
        switch($param){
            case '@alias':
                $alias = wf_var('WF_ALIAS', '');
                $temp_par = '@'.$alias['#name'];
                break;
            case '@tags':
                $temp_par = wf_var('WF_INPUT_TAG', '');
                break;
            case '@package':
                $temp_par = wf_var('WF_ASSET_PACKAGE','');
                break;
            case '@env':
                $temp_par = wf_var('WF_ENV','');
                break;
            case '@ver':
                $temp_par = wf_var('WF_ASSET_VER','');
                break;
            case '@make':
                $temp_par = wf_var('WF_ASSET_MAKE','');
                break;
            case '@asset':
                $temp_par = wf_var('WF_ASSET_NAME','no-artifact');
                break;
            case '@branch':
                $temp_par = wf_var('WF_ASSET_BRANCH','');
                break;
            case '@db-user':
                $temp_par = wf_var('WF_DB_USER','');
                break;
            case '@db-pwd':
                $temp_par = wf_var('WF_DB_PWD','');
                break;
            case '@db-name':
                $temp_par = wf_var('WF_DB_NAME','');
                break;
            default:
                $temp_par = $param;
                break;
        }

        if(isset($temp_par) && !empty($temp_par)){
            $parsed_par[$key] = $temp_par;
        }
//        ;;@alias - alias
//        ;;@package - dir_to_package
//        ;;@tags - config,init,revert plus user specified
//        ;;@env - environment
    }
    return $parsed_par;
}

/**
 * Prepare deploy_artifact action in profiler. Specifically, ensure the environment is set depending on the user provided info such as --make,--package,etc
 *
 */
function _prepare_deploy_action(&$target_action){
    //environment is determined based on the flag --make or --package
    if($target_action['name'] == 'deploy-asset' || $target_action['name'] == 'da'){

        if($env = wf_var('WF_ENV',FALSE)){
            //env already set
            drush_log('INFO: switching environment to "' . $env . '" environment since make file provided','notice');
            $target_action['option']['env'] = $env;
        }else{
            //if make-file specified, than deploy in stage
            if(file_exists(drush_get_context('WF_ASSET_MAKE'))){
                drush_log('INFO: switching environment to "stage" environment since make file provided','notice');
                drush_set_context('WF_ENV','stage');
                $target_action['option']['env'] = 'stage';
            }elseif(file_exists(drush_get_context('WF_ASSET_PACKAGE'))){
                //if package specified, then deploy in test
                drush_log('INFO: switching environment to "test" environment since package provided','notice');
                drush_set_context('WF_ENV','test');
                $target_action['option']['env'] = 'test';
            }else{
                //if neither package nor make specified, then check if asset has been specified
                $asset = drush_get_context('WF_ASSET_NAME');
                if(isset($asset) && !empty($asset)){
                    //run in dev
                    drush_log('INFO: switching environment to "dev" environment since asset is specified','notice');
                    drush_set_context('WF_ENV','dev');
                    $target_action['option']['env'] = 'dev';
                }

            }
        }

    }
}

/**
 * Prepares configure_asset action in profiler
 *
 * @param $target_action
 *      takes the target action to modify
 */
function _prepare_config_asset_action(&$target_action){
    if($target_action['name'] == 'configure-asset' || $target_action['name'] == 'ca'){
        //we switch the tags around to ensure config,init is called before user specified and then revert at the end
        $custom_tags = (isset($target_action['option']['tag']) && !empty($target_action['option']['tag'])) ? implode(',',$target_action['option']['tag']).',' : '' ;
        $target_action['option']['tag'] ='config,init,'.$custom_tags.'revert';
    }
}
