
<?php
require_once(dirname(__FILE__) . '/../lib/artifact-lib.inc');

function read_artifacts()
{
    $artifacts_file = _drush_config_file('home.drush', 'artifacts');
    if (!isset($artifacts_file) || empty($artifacts_file)) {
        $artifacts_file = _drush_config_file('custom', 'artifacts');
    }
    if (!isset($artifacts_file) || empty($artifacts_file)) {
        $artifacts_file = _drush_config_file('drupal', 'artifacts');
    }
    if (!isset($artifacts_file) || empty($artifacts_file)) {
        $artifacts_file = _drush_config_file('site', 'artifacts');
    }
    if (!isset($artifacts_file) || empty($artifacts_file)) {
        $artifacts_file = _drush_config_file('user', 'artifacts');
    }
    if (!isset($artifacts_file) || empty($artifacts_file)) {
        $artifacts_file = _drush_config_file('system', 'artifacts');
    }
    if(!isset($artifacts_file) || !file_exists($artifacts_file)){
        //see if its at WorkFlow default root
        $artifacts_file = wf_var('WF_ROOT').'/artifacts.drushrc.php';
    }


    if (isset($artifacts_file) && !empty($artifacts_file) && file_exists($artifacts_file)) {
        @include_once($artifacts_file);

        _set_artifact_defaults($artifacts);
        //check if artifacts specified
//        if(isset($artifact)){



        drush_set_context('WF_ARTIFACTS', $artifacts);
        drush_log('artifact file found and set in cache WF_ARTIFACTS', 'notice');
    } else {
        drush_log('ERROR: artifact configurations not found', 'notice');
    }
}

function _set_artifact_defaults(&$artifacts){
    if(!isset($artifacts) || empty($artifacts)){
        return Null;
    }else{
        //get the default attributes of the artifact
        $base_artifact = _base_artifact();
        //marge defaults
        //loop trough each artifact and set default values
        foreach ($artifacts as $key => $artifact) {
            //marge defaults
            $artifacts[$key] = $artifact + $base_artifact;
        }
    }
}


/*
 *
 */
function read_config()
{
    $config_file = _drush_config_file('home.drush', 'workflow');
    if (!isset($config_file) || empty($config_file)) {
        $config_file = _drush_config_file('custom', 'workflow');
    }
    if (!isset($config_file) || empty($config_file)) {
        $config_file = _drush_config_file('drupal', 'workflow');
    }
    if (!isset($config_file) || empty($config_file)) {
        $config_file = _drush_config_file('site', 'workflow');
    }
    if (!isset($config_file) || empty($config_file)) {
        $config_file = _drush_config_file('user', 'workflow');
    }
    if (!isset($config_file) || empty($config_file)) {
        $config_file = _drush_config_file('system', 'workflow');
    }

    //environment specific configurations
    if (isset($config_file) && !empty($config_file)) {
        @include_once($config_file);
    } else {
        drush_log(dt('ERROR: Env Configurations Not Set'), 'notice');
//        return drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: Env Configurations Not Set'));
    }

    $SERVER_HOME = drush_server_home();
    if (isset($workflow_config['root']) && !empty($workflow_config['root'])) {
        drush_set_context('WF_ROOT', $workflow_config['root']);
        drush_log(dt('WF_ROOT set to @name', array('@name' => $workflow_config['root'])), 'notice');
    } else {
        drush_set_context('WF_ROOT', $SERVER_HOME . '/workflow');
        drush_log(dt('ERROR: WF_ROOT  not specified. Using default $HOME/workflow'), 'notice');
//        return drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: WF_WORK_DIR  not specified'));
    }

    if (isset($workflow_config['work_dir']) && !empty($workflow_config['work_dir'])) {
        drush_set_context('WF_WORK_DIR', $workflow_config['work_dir']);
        drush_log(dt('WF_WORK_DIR set to @name', array('@name' => $workflow_config['work_dir'])), 'notice');
    } else {
        drush_set_context('WF_WORK_DIR', drush_get_context('WF_ROOT').'/work-dir');
        drush_log(dt('ERROR: WF_WORK_DIR  not specified. Using default $WF_ROOT/work-dir'), 'notice');
//        return drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: WF_WORK_DIR  not specified'));
    }

    if (isset($workflow_config['make_root']) && !empty($workflow_config['make_root'])) {
        drush_set_context('WF_MAKE_ROOT', $workflow_config['make_root']);
        drush_log(dt('WF_MAKE_ROOT set to @name', array('@name' => $workflow_config['make_root'])), 'notice');
    } else {
        drush_set_context('WF_MAKE_ROOT', drush_get_context('WF_ROOT').'/blueprints');
        drush_log(dt('ERROR: WF_MAKE_ROOT  not specified. Using default $WF_ROOT/blueprints'), 'notice');
//        return drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: WF_MAKE_ROOT  not specified'));
    }

    if (isset($workflow_config['dist']) && !empty($workflow_config['dist'])) {
        drush_set_context('WF_DIST', $workflow_config['dist']);
        drush_log(dt('WF_DIST set to @name', array('@name' => $workflow_config['dist'])), 'notice');
    } else {
        drush_set_context('WF_DIST', drush_get_context('WF_ROOT').'/dist');
        drush_log(dt('ERROR: WF_DIST  not specified. Using default $WF_ROOT/dist'), 'notice');
//        return drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: WF_DIST  not specified'));
    }

    if (isset($workflow_config['package_prefix']) && !empty($workflow_config['package_prefix'])) {
        drush_set_context('WF_PACKAGE_PREFIX', $workflow_config['package_prefix']);
        drush_log(dt('WF_PACKAGE_PREFIX set to @name', array('@name' => $workflow_config['package_prefix'])), 'notice');
    } else {
        drush_set_context('WF_PACKAGE_PREFIX', 'designssquare-com-');
        drush_log(dt('ERROR: WF_PACKAGE_PREFIX not specified. Using designssquare-com-'), 'notice');
//        return drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: WF_PACKAGE_PREFIX  not specified'));
    }

    if (isset($workflow_config['make_file']) && !empty($workflow_config['make_file'])) {
        drush_set_context('WF_DEFAULT_MAKE', $workflow_config['make_file']);
        drush_log(dt('WF_DEFAULT_MAKE set to @name', array('@name' => $workflow_config['make_file'])), 'notice');
    } else {
        drush_set_context('WF_DEFAULT_MAKE', drush_get_context('WF_MAKE_ROOT') . '/build-drupal-core.make');
        drush_log(dt('ERROR: WF_DEFAULT_MAKE are not set in external env configuration. Using make_root/build-drupal-core.make'), 'notice');
    }

    if (isset($workflow_config['db-user']) && !empty($workflow_config['db-user'])) {
        drush_set_context('WF_DB_USER', $workflow_config['db-user']);
        drush_log(dt('WF_DB_USER set to @name', array('@name' => $workflow_config['db-user'])), 'notice');
    } else {
        drush_set_context('WF_DB_USER', false);
        drush_log(dt('WF_DB_USER are not set in external env configuration. Using "root"'), 'notice');
    }

    if (isset($workflow_config['db-pwd']) && !empty($workflow_config['db-pwd'])) {
        drush_set_context('WF_DB_PWD', $workflow_config['db-pwd']);
        drush_log(dt('WF_DB_PWD set to @name', array('@name' => $workflow_config['db-pwd'])), 'notice');
    } else {
        drush_set_context('WF_DB_PWD', false);
        drush_log(dt('WF_DB_PWD are not set in external env configuration'), 'notice');
    }

    if (isset($workflow_config['db-name']) && !empty($workflow_config['db-name'])) {
        drush_set_context('WF_DB_NAME', $workflow_config['db_name']);
        drush_log(dt('WF_DB_NAME set to @name', array('@name' => $workflow_config['db-name'])), 'notice');
    } else {
        drush_set_context('WF_DB_NAME', false);
        drush_log(dt('WF_DB_NAME are not set in external env configuration'), 'notice');
    }

    if (isset($workflow_config['db-address']) && !empty($workflow_config['db-address'])) {
        drush_set_context('WF_DB_ADDRESS', $workflow_config['db-address']);
        drush_log(dt('WF_DB_ADDRESS set to @name', array('@name' => $workflow_config['db-address'])), 'notice');
    } else {
        drush_set_context('WF_DB_ADDRESS', false);
        drush_log(dt('WF_DB_ADDRESS are not set in external env configuration'), 'notice');
    }

    //artifact types declared
    if (isset($artifact_types) && !empty($artifact_types)) {
        drush_set_context('WF_ARTIFACTS_TYPES', $artifact_types);
        drush_log(dt('WF_ARTIFACTS_TYPES set to @name', array('@name' => implode(',', $artifact_types))), 'notice');
    } else {
        $artifact_types = array();
        $artifact_types[] = 'widget';
        $artifact_types[] = 'theme';
        drush_log(dt('WF_ARTIFACTS_TYPES not specified...setting to default: @types', array('@types' => implode(',', $artifact_types)), 'notice'));
    }

    //reads artifact external configurations
//    read_artifacts();
}

function load_workflow()
{
    //load depenedencies
    require_once(dirname(__FILE__) . '/artifact-lib.inc');
}

function load_slim_drupal()
{
    //load depenedencies
    require_once(dirname(__FILE__) . '/../drupal/includes/bootstrap.inc');
    require_once(dirname(__FILE__) . '/../drupal/includes/common.inc');
//        require(dirname(__FILE__) . '/../drupal/includes/utility.inc');
//    require(dirname(__FILE__) . '/../drupal/modules/system/system.module');
}

function load_drupal()
{
    // check if we can bootstrap
    $alias_record = drush_get_context('WF_ALIAS');

    if (empty($alias_record)) {
        drush_die("I can't bootstrap from the current location.", 0);
    }
    //load into drupal
    drush_set_context('DRUSH_SELECTED_DRUPAL_ROOT', $alias_record['root']);
    drush_bootstrap_max_to_sitealias($alias_record, DRUSH_BOOTSTRAP_DRUPAL_FULL);

    $errors = drush_get_context('DRUSH_BOOTSTRAP_ERRORS', array());
    if (!empty($errors)) {
        drush_log(dt('ERROR: cannot bootstrap into DRUSH_SELECTED_DRUPAL_ROOT. Error - @error', array('@error' => implode(',' . "\n", $errors))), 'notice');
    }

}

function init_inputs()
{
    $args = drush_parse_args();
    if (!empty($args)) {
        //handling first argument
        if (isset($args[1]) && !empty($args[1])) {
            switch ($args[0]) {
                case 'ea':
                case 'execute-action':
                    drush_set_context('WF_INPUT_ACTION', $args[1]);
                    drush_log(dt('WF_INPUT_ACTION set to @name', array('@name' => $args[1])), 'notice');
                    break;
                case 'pa':
                case 'profile-asset':
                    drush_set_context('WF_PROFILE_NAME', $args[1]);
                    drush_log(dt('WF_PROFILE_NAME set to @name', array('@name' => $args[1])), 'notice');
                    break;
                case 'ba':
                case 'build-asset':
                    drush_set_context('WF_ASSET_NAME', $args[1]);
                    drush_log(dt('WF_ASSET_NAME set to @name', array('@name' => $args[1])), 'notice');
                default:
                    //install-site
                    //configure-asset
                    //deploy-asset
                    $alias = drush_sitealias_get_record($args[1]);
                    drush_set_context('WF_ALIAS', $alias);
                    drush_log(dt('WF_ALIAS set to @name', array('@name' => implode(','."\n", $alias))), 'notice');
                    break;
            }
        }

        //handling second argument
        if (isset($args[2]) && !empty($args[2])) {
            switch ($args[0]) {
                case 'ba':
                case 'build-asset':
                    drush_set_context('WF_ASSET_TYPE', $args[2]);
                    drush_log(dt('WF_ASSET_TYPE set to @name', array('@name' => $args[2])), 'notice');
                    break;
                case 'pa':
                case 'profile-asset':
                    $alias = drush_sitealias_get_record($args[2]);
                    drush_set_context('WF_ALIAS', $alias);
                    drush_log(dt('WF_ALIAS set to @name', array('@name' => implode(','."\n", $alias))), 'notice');
                    break;
            }
        }

    }

    //--alias
    if($alias_name = drush_get_option('alias', FALSE)){
        $alias = drush_sitealias_get_record($alias_name);
        if(isset($alias)){drush_set_context('WF_ALIAS', $alias);};
        drush_log(dt('WF_ALIAS set to @name', array('@name' => implode(','."\n", $alias))), 'notice');

        //override defaults before user inputs
//        _override_defaults($alias);
    }

    drush_set_context('WF_ASSET_BRANCH', drush_get_option('branch', FALSE));
    drush_log(dt('WF_ASSET_BRANCH set to @name', array('@name' => drush_get_option('branch', FALSE))), 'notice');
    drush_set_context('WF_WIDGET_MIN', drush_get_option('min', FALSE));
    drush_log(dt('WF_WIDGET_MIN set to @name', array('@name' => drush_get_option('min', FALSE))), 'notice');
    drush_set_context('WF_ASSET_VER', drush_get_option('ver', FALSE));
    drush_log(dt('WF_ASSET_VER set to @name', array('@name' => drush_get_option('ver', FALSE))), 'notice');
    drush_set_context('WF_ENV', drush_get_option('env', 'stage'));
    drush_log(dt('WF_ENV set to @name', array('@name' => drush_get_option('env', FALSE))), 'notice');
    drush_set_context('WF_DEST', drush_get_option('dest',false));
    drush_log(dt('WF_DEST set to @name', array('@name' => drush_get_option('dest', FALSE))), 'notice');
//db related
    drush_set_context('WF_DB_NAME', drush_get_option('db-name', FALSE));
    drush_log(dt('WF_DB_NAME set to @name', array('@name' => drush_get_option('db-name', FALSE))), 'notice');
    drush_set_context('WF_DB_USER', drush_get_option('db-user', false));
    drush_log(dt('WF_DB_USER set to @name', array('@name' => drush_get_option('db-user', FALSE))), 'notice');
    drush_set_context('WF_DB_PWD', drush_get_option('db-pwd', false));
    drush_log(dt('WF_DB_PWD set to @name', array('@name' => drush_get_option('db-pwd', FALSE))), 'notice');
    drush_set_context('WF_DB_ADDRESS', drush_get_option('db-address', false));
    drush_log(dt('WF_DB_ADDRESS set to @name', array('@name' => drush_get_option('db-address', FALSE))), 'notice');

    //--asset
    if($article_name = drush_get_option('asset', FALSE)){
        drush_set_context('WF_ASSET_NAME', $article_name);
        drush_log(dt('WF_ASSET_NAME set to @name', array('@name' => $article_name)), 'notice');
    }


    $params = ($param = drush_get_option('param', false)) ? explode(',', $param) : array();
    drush_set_context('WF_INPUT_PARAM', $params);
    drush_log('WF_INPUT_PARAM is set to: ' . implode(',', $params), 'notice');

    $tags = ($tag = drush_get_option('tag', false)) ? explode(',', $tag) : array();
    drush_set_context('WF_INPUT_TAG', $tags);
    drush_log('WF_INPUT_TAG is set to: ' . implode(',', $tags), 'notice');

    $actions = ($action = drush_get_option('action', false)) ? explode(',', $action) : array();
    drush_set_context('WF_INPUT_ACTION', $actions);
    drush_log('WF_INPUT_ACTION is set to: ' . implode(',', $actions), 'notice');

    if ($package_dir = drush_get_option('package', FALSE)) {
        //check if dir specified
        if (!file_exists($package_dir)) {
            //if user didn't provided full path, lets see if its in the dist dir
            $package_dir = drush_get_context('WF_DIST') . '/' . $package_dir;
            if (!file_exists($package_dir)) {
                //incase the package is in the current directory
                $package_dir = getcwd() . '/' . $package_dir;
            }
        }

        //ensure full path
        $package_dir = realpath($package_dir);

        //check if compressed
        $package_ext = substr($package_dir,-3);
        if('.gz' == $package_ext || 'zip' == $package_ext){
            drush_log('package is compressed...uncompressing','notice');

            $package_temp_dest = drush_get_context('WF_ROOT').'/.cache/packages';
            if(!file_exists($package_temp_dest)){
                drush_log('cache didn\'t exist...creating', 'notice');
                execute_command('sudo mkdir -p '.$package_temp_dest);
            }

            if(file_exists($package_temp_dest.'/'.get_package_name($package_dir))){
                drush_log('..but first clean cache...removing ' . $package_temp_dest . '/'.get_package_name($package_dir));
                execute_command('sudo rm -R '.$package_temp_dest. '/' . get_package_name($package_dir));
            }

            if('.gz' == $package_ext){
                //untar
                drush_log('untar package into cache - '.$package_temp_dest,'notice');

                execute_command('sudo tar xvf '.$package_dir.' -C '.$package_temp_dest);
            }else{
                //unzip
                drush_log('unzip package into cache - '.$package_temp_dest,'notice');
                execute_command('sudo unzip '.$package_dir.' -d '.$package_temp_dest);
            }

            $package_dir = $package_temp_dest.'/'.get_package_name($package_dir);

        }
    }
    drush_set_context('WF_ASSET_PACKAGE', $package_dir);
    drush_log('WF_ASSET_PACKAGE is set to: ' . $package_dir, 'notice');



    drush_set_context('WF_ASSET_MAKE', drush_get_option('make', false));
    drush_log('WF_ASSET_MAKE is set to: ' . drush_get_option('make', false), 'notice');
}

function init_artifacts()
{
    /*
     *Handling Artifacts
     */
    //artifacts from external artifact configurations
    if ($art = wf_var('WF_ARTIFACTS', FALSE)) {
        $artifacts = $art;
        //retrieve artifact declaration from make file
        $artifacts_configured = get_artifacts_from_blueprint();
    } else {
        drush_log('External artifact configurations not set...loading from blueprint', 'debug');
        //retrieve artifact declaration from make file
        $artifacts = get_artifacts_from_blueprint();
        //add defaults
        _set_artifact_defaults($artifacts);
    }

    //get the default attributes of the artifact
//    $base_artifact = _base_artifact();

    //loop trough each artifact to mask out specific artifact configurations provided by make file
    foreach ($artifacts as $key => $artifact) {
        //marge defaults
//        $artifacts[$key] = $artifact + $base_artifact;

        //mask out configurations with ones specified in make file
        if (isset($artifacts_configured[$artifact['name']])) {
            $artifacts[$key] = $artifacts_configured[$artifact['name']] + $artifacts[$key];
        }
    }

    //reset the cache of artifacts with the updated configurations
    drush_set_context('WF_ARTIFACTS', $artifacts);
    drush_log('WF_ARTIFACTS: reset the cache of artifacts with the updated configurations ','notice');
}

function _base_artifact()
{
    $art = array();
    $art['name'] = '';
    $art['type'] = 'widget'; //type of the artifact - widget,theme,etc
    $art['repo'] = '';
    $art['ver'] = '';
    $art['repo_name'] = '';
    $art['final_name'] = '';
//    $art['work_dir'] = drush_get_context('WF_WORK_DIR', '');
    $art['work_dir'] = '';
    $art['work_dir_name'] = '';
    $art['branch'] = 'master';
    return $art;
}

/**
 * inform about which method ,out of the three possible, the make file is supplied
 */
function inform_make_supplied(){
    $name = wf_var('WF_ASSET_NAME','no-artifact');
    if($name != 'no-artifact'){
        //deploying by specifying asset
        drush_log('asset - "' . $name . '" specified...', 'ok');
    }elseIf($package = wf_var('WF_ASSET_PACKAGE',false)){
        //deploy in by providing package
        drush_log('asset specified from package ' . $package . '...', 'ok');
    }elseIf(drush_get_option('make', false)){
        drush_log('asset specified from make file ' . get_make_file() . '...', 'ok');
    }
//    else{
//        drush_die('ERROR: No blueprint not specified',0);
//    }

    $blueprint = get_make_file();
    drush_log('...using blueprint: ' . $blueprint, 'ok');
}

/**
 * retrieve db info
 * @param: $credential
 *      db credential such as db-name, db-pwd, db-user
 */

function get_db_credential($credential){
    $internal_var = _get_internal_name($credential);
    if($cred = wf_var($internal_var)){
        return $cred;
    }elseif(($alias = wf_var('WF_ALIAS')) && isset($alias[$credential])){
        return $alias[$credential];
    }else{
        drush_log('ERROR: db credential - '.$credential.' not specified', 'notice');
        return false;
    }
}

/**
 * retrieves destination based on alias or --dest flag
 */
function get_destination(){
    $dest = '';
    if($default_dest = wf_var('WF_DEST')){
        $dest = $default_dest;
    }elseif($alias = wf_var('WF_ALIAS')){
        $dest = $alias['root'];
    }else{
        $dest =  wf_var('WF_WORK_DIR') . '/deploys/no-dest-'.wf_var('WF_ASSET_NAME');
        drush_log('ERROR: destination not specified... defaults to '.$dest,'notice');
    }
    return $dest;
}

/**
 * retrieves environment
 */
function get_env(){
    $env = 'stage';
    if($env = wf_var('WF_ASSET_ENV', TRUE)){
        //$env not provided...see if specified in alias config.
        if(isset($site_record['env']) && !empty($site_record['env'])){
            $env = $site_record['env'];
        }
    }
    return $env;
}

/***
 * retrieves default make file
 */
function get_default_make_file(){
    $alias = wf_var('WF_ALIAS');
    if(isset($alias['make-file'])){
        return $alias['make-file'];
    }else{
        return wf_var('WF_DEFAULT_MAKE');
    }
}

/**mapping between user input and the internal var name
 * @param $user_input
 */
function _get_internal_name($user_input){
   $internal_name = '';
    switch($user_input){
        case 'db-user':
            $internal_name = 'WF_DB_USER';
            break;
        case 'db-pwd':
            $internal_name = 'WF_DB_PWD';
            break;
        case 'db-name':
            $internal_name = 'WF_DB_NAME';
            break;
        case 'db-address':
            $internal_name = 'WF_DB_ADDRESS';
            break;
        case 'make-file':
            $internal_name = 'WF_DEFAULT_MAKE';
            break;
        default:
            $internal_name = FALSE;
            break;
    }
    return $internal_name;
}

/**
 * overrides defaults of external environment variables
 */
function _override_defaults($alias){
    if(isset($alias) && !empty($alias)){
        foreach($alias as $key =>$value){
            $attribute = _get_internal_name($key);
            if($attribute){
                drush_log('Alias config ['.$key.'] = '.$value.' masked out value - ' . wf_var($attribute) . ' for [' . $attribute .'] = ', 'notice');
                drush_set_context($attribute, $value);
            }

        }
    }
}