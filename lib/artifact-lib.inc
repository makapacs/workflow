<?php
date_default_timezone_set('UTC');
$ts = date('Ymdhis');
//define("DEV_DIR", '/Users/maxit/Sites/drupal');
define('DEBUG_ON',TRUE);
//define('CONFIG_ROOT','/Users/maxit/Sites/drupal/config');

/******COMMON TASKS SHARED BETWEEN SCRIPTS*******/

//Check to see if we should back things up first.
if (drush_get_option('backup-files')) {
    print_r(args);
    drush_print("BACKING UP FILES...");
    drush_shell_exec('git commit -m "commit before running script on ' . $ts . '"');
    $output = drush_shell_exec_output();
    if ($output[0] === 'fatal: Not a git repository (or any of the parent directories): .git') {
        //make git repository and do initial commit
        drush_print('Git repository not setup, yet.....initiating one');
        drush_shell_exec('git init');
        drush_shell_exec('git add .');
        drush_shell_exec('git commit -m "initial commit before running script on ' . $ts . '"');
    }
//    print_r(drush_shell_exec_output());
    drush_print("File Backup complete.");
}


//@ToDo Consider removing this and calling drush_invoke_process directly
//processing make file
function _process_make_file($make_file, $dest = Null, $options = Null)
{
    if (!isset($options)) {
        print_r('options not set..');
        $options = array();
        $options['debug'] = drush_get_option('debug', FALSE);
        $options['no-core'] = drush_get_option('no-core', FALSE);
        $options['yes'] = TRUE;
        ($contr_dest = drush_get_option('contrib-destination', FALSE)) ? $options['contrib-destination'] = $contr_dest : '';
    }
    print("Debug - OPTIONS:");
    print_r($options);
//    print_r($make_file);
//    print_r('dest file: '.$dest);
    if ($dest) {
        print_r('make_file: '.$make_file. ' dest: '.$dest.' optionss:'.implode(',',$options));
        drush_print("SITE INSTALL");
        return drush_invoke_process('@none', 'make', array($make_file, $dest), $options);
    } else {
        drush_print("MODULE INSTALL");
        return drush_invoke_process('@self', 'make', array($make_file), $options);
    }

//    return ($dest) ? drush_invoke_process('@none', 'make', array($make_file, $dest), $options) : drush_invoke_process('@self', 'make', array($make_file), $options);
}

/**
 * Make a database backup
 */
function _backup_stuff()
{

    $dest = 'drush_backup';
    if (!file_exists($dest)) {
        drush_print("path to dest folder " . $dest . " does not exist. Creating...");
        drush_shell_exec('mkdir ' . $dest);
        drush_shell_exec('chrom -R 777 ' . $dest);
    }

    // backup the db
    $ts = date('Ymdhis');
    //@ToDo fix drush_invoke_process to work
    //(drush_invoke_process('@self', 'sql-dump', array('result-file' =>  'revert_backup.sql'))) ? drush_print('back up success') : drush_print('no back up') ;
    drush_shell_exec('drush sql-dump --result-file=' . $dest . '/revert_' . $ts . '_backup.sql --gzip=TRUE');
    print_r(drush_shell_exec_output());
}

function _import_db()
{
//    drush_shell_exec('drush sqlq --file=drush_backup/db_import.sql');
    drush_shell_exec('drush sql-cli < drush_backup/db_import.sql');
    print_r(drush_shell_exec_output());
}

function _drop_db()
{
    drush_shell_exec('drush sql-drop --yes');
    print_r(drush_shell_exec_output());
}

/*********PHP***********/
//temp solution till we upgrade to PHP5.5 then array_column() is available
function _array_column($input = null, $columnKey = null, $indexKey = null)
{
    // Using func_get_args() in order to check for proper number of
    // parameters and trigger errors exactly as the built-in array_column()
    // does in PHP 5.5.
    $argc = func_num_args();
    $params = func_get_args();

    if ($argc < 2) {
        trigger_error("array_column() expects at least 2 parameters, {$argc} given", E_USER_WARNING);
        return null;
    }

    if (!is_array($params[0])) {
        trigger_error('array_column() expects parameter 1 to be array, ' . gettype($params[0]) . ' given', E_USER_WARNING);
        return null;
    }

    if (!is_int($params[1])
        && !is_float($params[1])
        && !is_string($params[1])
        && $params[1] !== null
        && !(is_object($params[1]) && method_exists($params[1], '__toString'))
    ) {
        trigger_error('array_column(): The column key should be either a string or an integer', E_USER_WARNING);
        return false;
    }

    if (isset($params[2])
        && !is_int($params[2])
        && !is_float($params[2])
        && !is_string($params[2])
        && !(is_object($params[2]) && method_exists($params[2], '__toString'))
    ) {
        trigger_error('array_column(): The index key should be either a string or an integer', E_USER_WARNING);
        return false;
    }

    $paramsInput = $params[0];
    $paramsColumnKey = ($params[1] !== null) ? (string)$params[1] : null;

    $paramsIndexKey = null;
    if (isset($params[2])) {
        if (is_float($params[2]) || is_int($params[2])) {
            $paramsIndexKey = (int)$params[2];
        } else {
            $paramsIndexKey = (string)$params[2];
        }
    }

    $resultArray = array();

    foreach ($paramsInput as $row) {

        $key = $value = null;
        $keySet = $valueSet = false;

        if ($paramsIndexKey !== null && array_key_exists($paramsIndexKey, $row)) {
            $keySet = true;
            $key = (string)$row[$paramsIndexKey];
        }

        if ($paramsColumnKey === null) {
            $valueSet = true;
            $value = $row;
        } elseif (is_array($row) && array_key_exists($paramsColumnKey, $row)) {
            $valueSet = true;
            $value = $row[$paramsColumnKey];
        }

        if ($valueSet) {
            if ($keySet) {
                $resultArray[$key] = $value;
            } else {
                $resultArray[] = $value;
            }
        }

    }

    return $resultArray;
}

function array_orderby()
{
    $args = func_get_args();
    $data = array_shift($args);
    foreach ($args as $n => $field) {
        if (is_string($field)) {
            $tmp = array();
            foreach ($data as $key => $row)
                $tmp[$key] = $row[$field];
            $args[$n] = $tmp;
        }
    }
    $args[] = & $data;
    call_user_func_array('array_multisort', $args);
    return array_pop($args);
}

/***********Stage ***********/
/*
 * param: $dest - the location for custom modules without traling slash
 */
function stage_modules($artifact_type, $artifact_name, $dest = NULL, $debug = TRUE)
{

    $git_util_repo = 'https://github.com/kapasoft-config-scripts/';
    $git_module_repo = 'https://github.com/kapasoft-drupal-modules/';
    $module_dest_dir = (isset($dest)) ? $dest . '/' : 'sites/all/modules/';

    //retrieving modules to be included in the build
    $make_file = '../../config/builds/' . $artifact_type . '-builds/designssquare_com_' . $artifact_name . '_' . $artifact_type . '.make';
    $build_file_parsed = drupal_parse_info_file($make_file);
    $widget_modules = _array_column(array_orderby($build_file_parsed['widget'], 'order', SORT_ASC), "name");

    /*****DEBUG******/
    if (DEBUG_ON) {
        drush_print('make file: ' . $make_file);
        drush_print('Modules to include:');
        print_r($widget_modules);
    }

    foreach ($widget_modules as $key => $module) {
        $widget_name = null;
        switch ($module) {
            case 'designssquare_lib':
                $repo_name = 'designssquare-utils.git';
                $source_repo = $git_util_repo . $repo_name;
                $widget_name = 'designssquare_com_lib';
                break;
            case  'designssquare_alias_path':
                $repo_name = 'designssquare_alias_path.git';
                $source_repo = $git_util_repo . $repo_name;
                $widget_name = 'designssquare_lib_alias';
                break;
            case  'designssquare_lib_assets':
                $repo_name = 'designssquare-lib-assets.git';
                $source_repo = $git_util_repo . $repo_name;
                $widget_name = 'designssquare_lib_assets';
                break;
            default:
                if ($artifact_type == 'theme') {
                    //installing theme's module
                    $repo_name = $artifact_name . '.git';
                    $source_repo = $git_module_repo . $repo_name;
                    $widget_name = 'designssquare_com_module_' . $artifact_name;
                }
                break;
        }

        if (isset($widget_name)) {
            if (is_dir($module_dest_dir . $widget_name)) {
                drush_print('Removing module ' . $widget_name . ' before staging...');
                drush_shell_exec('sudo rm -R ' . $module_dest_dir . $widget_name);
                print_r(drush_shell_exec_output());
            }
            drush_print('staging ' . $widget_name . ' module ...' . $source_repo);
            //@ToDo This is outdated
            drush_shell_exec('sudo git clone ' . $source_repo . ' ' . $module_dest_dir . $widget_name);
            print_r(drush_shell_exec_output());
        }
    }
}

/************Building Dist***********/

/*
 * takes artifact info and clones into destination based on the tag name versions
 *
 * @param: $dest
 *      path to the destination dir the artifacts is cloned into
 *
 */

function build_artifact($artifact, $dest){
    $artifact_config = get_artifact_by_name($artifact['name']);
    //artifact name in the makefile is different than one in artifact configuration or Artifact Config cache is not set.
    if(!isset($artifact_config) || empty($artifact_config)){
        drush_log('ERROR: artifact name mismatch - or - artifact cache not set', 'notice');
        $artifact_config = $artifact;
    }

    //keep it consistent with drupal naming and file structure
    $type = ($artifact_config['type'] == 'widget') ? 'module' : $artifact_config['type'];

    $widget_dist_dest_lib = $dest . '/' . $type . 's/' . $artifact_config['final_name'];
    //@ToDo make it clone the tag as well to avoid separate co for tag
    $command = 'sudo git clone -b ' . $artifact_config['branch'] . ' ' . $artifact_config['repo'] . $artifact_config['repo_name'] . '.git ' . $widget_dist_dest_lib;
    drush_log(dt('cloning ...@repo', array('@repo' => $artifact_config['repo'])),'ok');
    execute_command($command);

    if(isset($artifact_config['ver']) && !empty($artifact_config['ver'])){
        $command = 'sudo git checkout '.$artifact_config['ver'];
        drush_log(dt('checking out version...@ver', array('@ver' => $artifact_config['ver'])), 'debug');
        $cur_dir = getcwd();
        drush_op('chdir', $widget_dist_dest_lib);
        execute_command($command);
        drush_op('chdir', $cur_dir);
    }
}

//@ToDo remove
//function build_modules($artifact_type, $artifact_name, $dest, $debug = TRUE)
//{
//    $git_util_repo = 'https://github.com/kapasoft-config-scripts/';
//    $git_module_repo = 'https://github.com/kapasoft-drupal-modules/';
//    //retrieving modules to be included in the build
//    $make_file = get_make_file($artifact_name, $artifact_type, 'dev');
////    drush_print('****make-file: ' . $make_file);
//    $build_file_parsed = drupal_parse_info_file($make_file);
//    $widget_modules = _array_column(array_orderby($build_file_parsed['widget'], 'order', SORT_ASC), "name");
//
//    /*****DEBUG******/
//    if (DEBUG_ON) {
//        drush_print('Modules to include:');
//        print_r($widget_modules);
//    }
//
//    foreach ($widget_modules as $key => $module) {
//
//        $artifact_ins = get_artifact_by_name($module);
//        $widget_dependency_name = $artifact_ins['name'];
//        $widget_dist_dest_lib = $dest . '/modules/' . $artifact_ins['final_name'];
//        drush_print('building ' . $widget_dependency_name . ' module ...' . $artifact_ins['repo']);
//        drush_shell_exec('git clone -b ' . $artifact_ins['branch'] . ' ' . $artifact_ins['repo'] . $artifact_ins['repo_name'] . '.git ' . $widget_dist_dest_lib);
//        print_r(drush_shell_exec_output());
//
//    }
//}

function _copy_documentation($src, $dest)
{
//    drush_print('copying documentation ....' . $dest . '/docs/* from ' . $src . '/docs/*');
//    drush_shell_exec('sudo mkdir ' . $dest . '/docs/');
    execute_command('sudo mkdir ' . $dest . '/docs/');
//    print_r(drush_shell_exec_output());
//    drush_shell_exec('sudo sudo cp -R ' . $src . '/docs/* ' . $dest . '/docs/');
    execute_command('sudo sudo cp -R ' . $src . '/docs/* ' . $dest . '/docs/');
//    print_r(drush_shell_exec_output());
}

/**********Configure*********/

function configure_ckeditor($module_name = 'designssquare_com_ckeditor')
{
//    $artifact_dir = drupal_get_path('module', $module_name);
//    $artifact_dir_name = array_pop(explode( "/", $artifact_dir));
    $artifact = get_artifact_by_name($module_name);
//    drush_print("dir_name:".$artifact_dir_name);

//Link CKeditor
    drush_print('Linking CKeditor library....');
    $editor_dest = 'sites/all/libraries/ckeditor';
    if (!file_exists('sites/all/libraries')) {
        drush_print('sites/all/libraries directory does not exist...creating one');
        drush_shell_exec('sudo mkdir sites/all/libraries');
        print_r(drush_shell_exec_output());
    }

    if (file_exists('sites/all/libraries/ckeditor')) {
        drush_print('sites/all/libraries/ckeditor directory already exist...removing');
        drush_shell_exec('sudo rm -R sites/all/libraries/ckeditor');
        print_r(drush_shell_exec_output());
    }
    $editor_src = '../modules/' . $artifact['final_name'] . '/libraries/ckeditor';
//$editor_src = $artifact_dir.'/libraries/ckeditor';
    drush_print('linking ckeditor ' . $editor_dest . ' to path ' . $editor_src);
    (symlink($editor_src, $editor_dest)) ? 'linking widget named ' . $editor_dest . ' to path ' . $editor_src : "***ERROR: didn't limked to" . $editor_src;


    //@ToDo set profiles for IMCE to work
    $imce_roles_profiles = array(
        4 =>
        array(
            'weight' => '0',
            'public_pid' => '1',
        ),
        3 =>
        array(
            'weight' => '0',
            'public_pid' => '1',
        ),
        2 =>
        array(
            'weight' => 11,
            'public_pid' => '1',
        ),
        1 =>
        array(
            'weight' => 12,
            'public_pid' => 0,
        ),
    );
    variable_set('imce_roles_profiles', $imce_roles_profiles);


}


function configure_audio($module_name)
{
//    $artifact_dir = drupal_get_path('module', $module_name);
//    $artifact_dir_name = array_pop(explode( "/", $artifact_dir));
    $artifact = get_artifact_by_name($module_name);
    //configure audio
    variable_set('audiofield_audioplayer', "wpaudioplayer");
    $payer_audio_src = "sites/all/modules/" . $artifact['final_name'] . "/libraries/player";
    if (!file_exists($payer_audio_src)) {
        drush_die("path to audo player lib does not exist: " . $payer_audio_src, 0);
    }
    variable_set('audiofield_players_dir', $payer_audio_src);
    drush_print('...configured audio player lib to path ' . $payer_audio_src);

}

/****
 *
 * @param: --mail
 * email account needs to be specified
 *
 * @param: --password
 * the passowrd of the new user
 *
 * @param --role
 * the role of the new user
 *
 * @param(direct): user_name
 * the name of user is added
 *
 *
 *
 *
 *****/
function add_user($user_name)
{
//    $email = variable_get('mail', 'admin@domain.com');
//    $password = variable_get('password', 'password1');
    $role = drush_get_option('role', 'anonymous user');

    drush_user_create($user_name);
    drush_user_add_role($role, $user_name);
}

//retrieve the name of widget
function get_widget_name($name, $type)
{
    $artifact = get_artifact_by_name($name);
    if (!empty($artifact['name'])) {
        $artifact_name = $artifact['final_name'];
    } else {
        $artifact_name = 'designssquare_com_' . $type . '_' . $name;
    }
    return $artifact_name;
}

function get_repo($name, $type)
{
    $repo_name = '';
    $repo_list = get_repos();
    switch ($type) {
        case 'widget':
        case 'module':
            $repo_name = $repo_list['module'] . $name . '.git';
            break;
        case 'theme':
            $repo_name = $repo_list['theme'] . $name . '.git';
            break;
        case 'site':
            $repo_name = $repo_list['site'] . $name . '.git';
            break;
        default:
            $repo_name = $repo_list['utils'] . $name . '.git';
            break;
    }
    return $repo_name;
}

/*
 * retrieves the artifacts from make file based on the type(widget, theme, etc)

 * @param: $type
 *      type of the widget for retrieving makefile as well as specific artifacts from makefile afterwards
 *
 * @return: array
 *      array of artifacts from makefile of certain type(i.e. widget, theme,etc)
 */
function get_artifacts_from_blueprint($type = 'all')
{
    //retrieve name and type for reading make file
    $widget_name = drush_get_context('FLOW_WIDGET_NAME');

    //list of artifacts in the package
    $artifacts = array();

    //add dependent artifacts
    //read make file
    //retrieving artifacts to be included in the build
    $make_file = get_make_file($widget_name);

    $build_file_parsed = drupal_parse_info_file($make_file);

    $artifacts_configured = array();
    if($type != 'all'){
        $artifacts_configured = (isset($build_file_parsed[$type])) ? array_orderby($build_file_parsed[$type], 'order', SORT_ASC) : array();
    }else{
            foreach(_all_artifact_types() as $key => $config_type){
            $artifacts_configured = (isset($build_file_parsed[$config_type])) ? array_merge($artifacts_configured, array_orderby($build_file_parsed[$config_type], 'order', SORT_ASC)) : $artifacts_configured;
        }
    }

//    if ($type != 'module') {
        //clean out dependencies part of the widget and doesn't need to be build
//        foreach ($artifacts_configured as $key => $artifact) {
//            $art = get_artifact_by_name($artifact['name']);
//            if (!empty($art['name'])) {
//                //if artifact not
//                $artifacts[] = $artifact;
//            }
//        }
//    }

    return $artifacts_configured;
}

//retrieves list of actions from the make file
function get_actions_from_make($make_file)
{
    if (!file_exists($make_file)) {
        drush_die('make file - ' . $make_file . ' does not exist');
    }

    $build_file_parsed = drupal_parse_info_file($make_file);
    $actions = array_orderby($build_file_parsed['action'], 'order', SORT_ASC);

    return $actions;
}


//function all_themes_from_package($name, $type)
//{
//    //add the main theme
//    $themes = array();
//
//    //add dependent themes
//    //read make file
//    //retrieving modules to be included in the build
//    $make_file = get_make_file($name, $type);
////        drush_print('****make-file: '.$make_file);
//    if (!file_exists($make_file)) {
//        drush_die('make file - ' . $make_file . ' does not exist');
//    }
//    $build_file_parsed = drupal_parse_info_file($make_file);
////        drush_print('***parsed_make: ');
////        print_r($build_file_parsed);
//    $themes_configured = _array_column(array_orderby($build_file_parsed['theme'], 'order', SORT_ASC), "name");
//
//    //clean out dependencies part of the widget and doesn't need to be build
//    foreach ($themes_configured as $key => $m_name) {
//        $art = get_artifact_by_name($m_name);
//        if (!empty($art['name'])) {
//            $themes[] = $m_name;
//        }
//    }
//
//
//    return $themes;
//}
//
//function all_widget_modules($name, $type)
//{
//    $min = drush_get_option('min-ver', 'no');
//    $is_min = ($min == 'yes') ? true : false;
//    $widget_modules = array();
//
//    if (!$is_min) {
//        //add dependencies
//
//        //read make file
//        //retrieving modules to be included in the build
//        $make_file = get_make_file($name, $type);
//        if (!file_exists($make_file)) {
//            drush_die('make file - ' . $make_file . ' does not exist');
//        }
//        $build_file_parsed = drupal_parse_info_file($make_file);
//        $widget_modules_configured = _array_column(array_orderby($build_file_parsed['widget'], 'order', SORT_ASC), "name");
//
//        //clean out dependencies part of the widget and doesn't need to be build
//        foreach ($widget_modules_configured as $key => $m_name) {
//            $art = get_artifact_by_name($m_name);
//            if (!empty($art['name'])) {
//                $widget_modules[] = $m_name;
//            }
//        }
//    } else {
//        //add the main module
//        $widget_modules[] = $name;
//    }
//
//    drush_print('Widget Modules:');
//    print_r($widget_modules);
//    return $widget_modules;
//}

//retrieves make file or returns false if cannot find the name
//on default make_dir is '$type-builds'. Otherwise, it is specified in artifact configurations
//on defautl make_name is built from alternatives. Otherwise, it is specified in artifact configurations
function get_make_file($name)
{
    $config = get_config();
    $artifact = get_artifact_by_name($name);

    $list_of_names = artifact_names($name, $artifact['type']);
    foreach($list_of_names as $key => $artifact_name){
        $make_file = $config['make_root'].'/';
        $make_file .=  $artifact['type'].'s/';
        $make_file .=  $artifact_name.'.make';

        if(file_exists($make_file)){
            drush_log('make_file found ' . $make_file, 'notice');
            return $make_file;
        }
    }

//    return drush_set_error('WORKFLOW_MAKE_ERROR', dt('ERROR: make_file '.$make_file.' not found' ));
    drush_log(dt('Artifact blueprint(make file) was not found. Using default - ' . $config['default_make']),'notice');
    return $config['default_make'];
}

//@ToDo - consider removing and using the drush_get_context() API
function get_config()
{
    $config = array();
    $config['work_dir'] = ($work_dir = drush_get_context('FLOW_CONFIG_WORK_DIR', FALSE)) ? $work_dir : drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: FLOW_CONFIG_WORK_DIR  not specified' ));
    $config['make_root'] = ($make_root = drush_get_context('FLOW_CONFIG_MAKE_ROOT', FALSE)) ? $make_root : drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: FLOW_CONFIG_MAKE_ROOT  not specified'));
    $config['dist'] = ($dist = drush_get_context('FLOW_CONFIG_DIST', FALSE)) ? $dist : drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: FLOW_CONFIG_DIST  not specified' ));
    $config['package_prefix'] = ($prefix = drush_get_context('FLOW_CONFIG_PACKAGE_PREFIX')) ? $prefix : drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: FLOW_CONFIG_PACKAGE_PREFIX  not specified' ));
    $config['default_make'] = ($make = drush_get_context('FLOW_CONFIG_DEFAULT_MAKE')) ? $make : drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: FLOW_CONFIG_DEFAULT_MAKE  not specified' ));
    $config['db_user'] = ($make = drush_get_context('FLOW_DB_USER')) ? $make : drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: FLOW_DB_USER  not specified' ));
    $config['db_pwd'] = ($make = drush_get_context('FLOW_DB_PWD')) ? $make : drush_set_error('WORKFLOW_INIT_CONFIG_ERROR', dt('ERROR: FLOW_DB_PWD  not specified' ));
    return $config;
}

function remove_cur_artifact($name, $type)
{
    $alias = drush_get_context('FLOW_WIDGET_ALIAS');
    $root = $alias['root'];
    $artifact = get_artifact_by_name($name);

    $type = ($type != 'theme') ? 'module' : $type;
    if(isset($artifact) && !empty($artifact) && isset($artifact['final_name']) && !empty($artifact['final_name'])){
        $dest_dir = $root . '/sites/all/' . $type . 's/' . $artifact['final_name'];
    }else{
        //we may be quering the artifact by final name instead the actual name when reading from current drupal instance
        $dest_dir = $root . '/sites/all/' . $type . 's/' .$name;
    }
//    drush_log('remove_cur_artifiact checks destination: '.$dest_dir, 'notice');
    if (file_exists($dest_dir)) {
        drush_log('deleting existing ' . $type . ': ' . $name . '....' . $dest_dir, 'notice');
        execute_command('sudo rm -R ' . $dest_dir);
    }else{
        drush_log('ERROR: artifact - '.$dest_dir.' to be removed doesn\'t exist', 'notice');
    }
}

function get_artifact_by_name($name)
{
//    $list_of_repos = get_repos();
//    $artifact = array();
//    $artifact['name'] = '';
//    $artifact['type'] = 'module'; //type of the widget - module or theme...no site,etc
//    $artifact['repo'] = '';
//    $artifact['repo_name'] = '';
//    $artifact['final_name'] = '';
//    $artifact['dev_dir'] = 'modules'; //relative dev directory
//    $artifact['dev_name'] = $name; //name of the widget in dev environment
//    $artifact['branch'] = 'master';
//    $artifact['make_dir'] = ''; //relative path to make file directory. Default '$type-builds'
//    $artifact['make_name'] = ''; //name of the make file. Default built from artifact name

//    switch ($name) {
//        case 'designssquare_lib':
//        case 'designssquare_com_lib':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['utils'];
//            $artifact['repo_name'] = 'designssquare-utils';
//            $artifact['final_name'] = 'designssquare_com_lib';
//            $artifact['dev_name'] = 'designssquare_lib';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'designssquare_alias_path':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['utils'];
//            $artifact['repo_name'] = 'designssquare_alias_path';
//            $artifact['final_name'] = 'designssquare_com_lib_alias';
//            $artifact['dev_name'] = 'designssquare_alias_path';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'designssquare_lib_assets':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['utils'];
//            $artifact['repo_name'] = 'designssquare-lib-assets';
//            $artifact['final_name'] = 'designssquare_com_lib_assets';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'designssquare_lib_feature_assets':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['utils'];
//            $artifact['repo_name'] = 'designssquare-lib-feature-assets';
//            $artifact['final_name'] = 'designssquare_com_lib_feature_assets';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'designssquare_lib_sample_data':
//        case 'designssquare_com_lib_sample_data':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['utils'];
//            $artifact['repo_name'] = 'designssquare-lib-sample-data';
//            $artifact['final_name'] = 'designssquare_com_lib_sample_data';
//            $artifact['dev_name'] = 'designssquare_lib_sample_data';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case 'designssquare_theme_blocks':
//        case 'designssquare_com_theme_blocks':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'designssquare_com_theme_blocks';
//            $artifact['final_name'] = 'designssquare_com_theme_blocks';
//            $artifact['dev_name'] = 'designssquare_theme_blocks';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case 'designssquare_com_theme_blocks_plus':
//        case 'designssquare_theme_blocks_plus':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'designssquare_com_theme_blocks';
//            $artifact['final_name'] = 'designssquare_com_theme_blocks_plus';
//            $artifact['branch'] = 'plus';
//            $artifact['dev_name'] = 'designssquare_com_theme_blocks_plus';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'blog':
//        case  'designssquare_com_blog':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'blog';
//            $artifact['final_name'] = 'designssquare_com_widget_blog';
//            $artifact['dev_name'] = 'blog';
//            $artifact['dev_dir'] = 'modules';
////            $artifact['make_dir'] = 'module-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_blog_module';
//            break;
//        case 'ckeditor':
//        case 'designssquare_com_utils_ckeditor':
//        case 'designssquare_com_ckeditor':
////        case 'designssquare_utils_ckeditor':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['utils'];
//            $artifact['repo_name'] = 'designssquare-utils-ckeditor';
//            $artifact['final_name'] = 'designssquare_com_utils_ckeditor';
//            $artifact['dev_name'] = 'designssquare_utils_ckeditor';
//            break;
//        case 'commerce':
//        case  'designssquare_com_commerce':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'commerce';
//            $artifact['final_name'] = 'designssquare_com_widget_commerce';
//            $artifact['dev_name'] = 'commerce';
//            $artifact['dev_dir'] = 'modules';
////            $artifact['make_dir'] = 'module-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_commerce_module';
//            break;
//        case 'gallery':
//        case  'designssquare_com_widget_gallery':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'gallery';
//            $artifact['final_name'] = 'designssquare_com_widget_gallery';
//            $artifact['dev_name'] = 'gallery';
//            $artifact['dev_dir'] = 'modules';
////            $artifact['make_dir'] = 'module-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_gallery_module';
//            break;
//        case 'slider':
//        case 'designssquare_com_widget_slider':
//        case  'designssquare_com_slider':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'slider';
//            $artifact['final_name'] = 'designssquare_com_widget_slider';
//            $artifact['dev_name'] = 'slider';
//            $artifact['dev_dir'] = 'modules';
////            $artifact['make_dir'] = 'module-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_slider_module';
//            break;
//        case 'newsletter':
//        case  'designssquare_com_widget_newsletter':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'newsletter';
//            $artifact['final_name'] = 'designssquare_com_widget_newsletter';
//            $artifact['dev_name'] = 'newsletter';
//            $artifact['dev_dir'] = 'modules';
////            $artifact['make_dir'] = 'module-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_newsletter_module';
//            break;
//        case 'podcast':
//        case  'designssquare_com_widget_podcast':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'podcast';
//            $artifact['final_name'] = 'designssquare_com_widget_podcast';
//            $artifact['dev_name'] = 'podcast';
//            $artifact['dev_dir'] = 'modules';
////            $artifact['make_dir'] = 'module-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_podcast_module';
//            break;
//        case 'portfolio':
//        case  'designssquare_com_widget_portfolio':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'portfolio';
//            $artifact['final_name'] = 'designssquare_com_widget_portfolio';
//            $artifact['dev_name'] = 'portfolio';
//            $artifact['dev_dir'] = 'modules';
////            $artifact['make_dir'] = 'module-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_portfolio_module';
//            break;
//        case 'service':
//        case 'designssquare_com_widget_service':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'service';
//            $artifact['final_name'] = 'designssquare_com_widget_service';
//            $artifact['dev_name'] = 'service';
//            $artifact['dev_dir'] = 'modules';
////            $artifact['make_dir'] = 'module-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_portfolio_module';
//            break;
//        case  'espresso':
//            $artifact['name'] = $name;
//            $artifact['type'] = 'theme';
//            $artifact['repo'] = $list_of_repos['theme'];
//            $artifact['repo_name'] = 'espresso';
//            $artifact['final_name'] = 'espresso';
//            $artifact['dev_dir'] = 'themes';
//            $artifact['dev_name'] = 'espresso';
////            $artifact['make_dir'] = 'theme-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_espresso_theme';
//            break;
//        case  'espresso_theme':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'espresso';
//            $artifact['final_name'] = 'designssquare_com_theme_espresso';
//            $artifact['dev_name'] = 'espresso_theme';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'espresso_theme_plus':
//        case 'desigssquare_com_theme_espresso_plus':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'espresso';
//            $artifact['final_name'] = 'desigssquare_com_theme_espresso_plus';
//            $artifact['branch'] = 'plus';
//            $artifact['dev_name'] = 'espresso_theme_plus';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'bazar':
//            $artifact['name'] = $name;
//            $artifact['type'] = 'theme';
//            $artifact['repo'] = $list_of_repos['theme'];
//            $artifact['repo_name'] = 'bazar';
//            $artifact['final_name'] = 'bazar';
//            $artifact['dev_dir'] = 'themes';
//            $artifact['dev_name'] = 'bazar';
////            $artifact['make_dir'] = 'theme-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_espresso_theme';
//            break;
//        case  'bazar_theme':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'bazar';
//            $artifact['final_name'] = 'designssquare_com_theme_bazar';
//            $artifact['dev_name'] = 'bazar_theme';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'builder':
//            $artifact['name'] = $name;
//            $artifact['type'] = 'theme';
//            $artifact['repo'] = $list_of_repos['theme'];
//            $artifact['repo_name'] = 'builder';
//            $artifact['final_name'] = 'builder';
//            $artifact['dev_dir'] = 'themes';
////            $artifact['make_dir'] = 'theme-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_builder_theme';
//            break;
//        case  'builder_theme':
//        case 'designssquare_com_theme_builder':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'builder';
//            $artifact['final_name'] = 'designssquare_com_theme_builder';
//            $artifact['dev_name'] = 'builder_theme';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case 'builder_theme_plus':
//        case 'desigssquare_com_theme_builder_plus':
//            $artifact['name'] = $name;
//            $artifact['repo'] = $list_of_repos['module'];
//            $artifact['repo_name'] = 'builder';
//            $artifact['final_name'] = 'desigssquare_com_theme_builder_plus';
//            $artifact['branch'] = 'plus';
//            $artifact['dev_name'] = 'builder_theme_plus';
//            $artifact['dev_dir'] = 'modules';
//            break;
//        case  'bootstrap':
//            $artifact['name'] = $name;
//            $artifact['type'] = 'theme';
//            $artifact['repo'] = $list_of_repos['theme'];
//            $artifact['repo_name'] = 'bootstrap';
//            $artifact['final_name'] = 'bootstrap';
//            $artifact['dev_dir'] = 'themes';
//            break;
//        case 'church':
//        case  'designssquare_com_site_church':
//            $artifact['name'] = $name;
//            $artifact['type'] = 'module';
//            $artifact['repo'] = $list_of_repos['site'];
//            $artifact['repo_name'] = 'church';
//            $artifact['final_name'] = 'designssquare_com_site_church';
//            $artifact['dev_dir'] = 'modules-site';
////            $artifact['make_dir'] = 'site-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_church_site';
//            break;
//        case 'loam':
//        case  'designssquare_com_site_loam':
//            $artifact['name'] = $name;
//            $artifact['type'] = 'module';
//            $artifact['repo'] = $list_of_repos['site'];
//            $artifact['repo_name'] = 'loam';
//            $artifact['final_name'] = 'designssquare_com_site_loam';
//            $artifact['dev_dir'] = 'dev-sites';
//            $artifact['dev_dir'] = 'modules-site';
////            $artifact['make_dir'] = 'site-builds'; //relative path to make file directory
////            $artifact['make_name'] = 'designssquare_com_church_site';
//            break;
//        default:
////            _log('ERROR: Artifact - '.$name.' not declared');
//            drush_log(dt('ERROR: Artifact - @name not declared',array('@name' => $name)), 'notice');
//            $artifact['final_name'] = 'NOT-FOUND';
//
//    }

    //for masking out defautl artifact conigurations over the ones in the make file
//    $artifacts_configured = get_artifacts_from_blueprint();
//    $artifact = $artifacts_configured['name'];

    if($art = drush_get_context('FLOW_ARTIFACTS', FALSE)){
        drush_log('artifact declarations masked out ','debug');
        return $art[$name];
//        print_r('@before');
//        print_r($artifact);
//        $artifact = $artifact + $art[$name];
//        print_r('@after');
//        print_r($artifact);
    }else{
      return drush_set_error('DRUSH_ARTIFACT_INIT_FAILED', dt('FLOW_ARTIFACTS are not set' ));

        //build from make file
//        $artifacts = get_artifacts_from_blueprint();
//        print_r('@@new FLOW_ARTIFACTS set');
//        print_r($artifacts);
    }

//    return $artifact;
}

//@ToDo Consider remove since external artifact configurations are present
function get_repos()
{
    $repo_list = array();
    $repo_list['module'] = 'https://github.com/kapasoft-drupal-modules/';
    $repo_list['theme'] = 'https://github.com/kapasoft-drupal-themes/';
    $repo_list['site'] = 'https://github.com/kapasoft-drupal-site-modules/';
    $repo_list['utils'] = 'https://github.com/kapasoft-config-scripts/';
    return $repo_list;
}

//retrieve projects modules from the make file
function _project_modules_from_make($make_file_parsed)
{
    $projects_final = array();
    foreach ($make_file_parsed['projects'] as $key => $project) {
        if ($key != 'drupal') {

            if (is_array($project) && isset($project['version'])) {
                $projects_final[] = $key . '-7.x-' . $project['version'];
            } else {
                $projects_final[] = $make_file_parsed['projects'][$key];
            }
        }
    }
    return array_values($projects_final);
}

function _all_artifact_types()
{
    $artifact_types = ($types = drush_get_context('FLOW_ARTIFACTS_TYPES', FALSE)) ? $types : array('widget','theme');
    return $artifact_types;
}

//removing artifacts based on whats in package
function remove_package_artifacts($dist_package, $type)
{
    //removing artifacts based on whats in the package
    $type = ($type != 'theme') ? 'module' : $type ;
    $handle = opendir($dist_package . '/' . $type . 's/');
    while ($handle && false !== ($module = readdir($handle))) {
        if ($module !== '.' && $module !== '..') {
            drush_log('deleting...'.$module, 'notice');
            remove_cur_artifact($module, $type);
        }
    }
    closedir($handle);
}

//removes distribution usually when rebuilding a new one
function remove_distribution($package){
//    (DEBUG_ON) ? drush_print('Build: ' . $package['name'] . ' exists...deleting') : '';
    drush_log(dt('build - @package exists...deleting', array('@package' => $package['name'])), 'notice');
    execute_command('sudo rm -R ' . $package['path'].'/'.$package['name']);
//    drush_shell_exec('sudo rm -R ' . $package['path'].'/'.$package['name']);
//    drush_log(dt('output: @output',array('@output' => implode(',',drush_shell_exec_output()))),'debug');
//    (DEBUG_ON) ? print_r(drush_shell_exec_output()) : '';

    //removing old zip file as well
    execute_command('sudo rm ' . $package['path'].'/'.$package['name'] . '.tar.gz');
//    drush_shell_exec('sudo rm ' . $package['path'].'/'.$package['name'] . '.tar.gz');
//    (DEBUG_ON) ? print_r(drush_shell_exec_output()) : '';

    //removing old zip file as well
    execute_command('sudo rm ' . $package['path'].'/'.$package['name'] . '.zip');
//    drush_shell_exec('sudo rm ' . $package['path'].'/'.$package['name'] . '.zip');
//    (DEBUG_ON) ? print_r(drush_shell_exec_output()) : '';
}

//removing artifacts based on whats in the make file
function remove_make_artifacts($type){
    foreach (get_artifacts_from_blueprint($type) as $key => $artifact) {
        drush_log(dt('Removing From Make File...artifact_name:'.$artifact['name']), 'notice');
        remove_cur_artifact($artifact['name'], $type);
    }
}

//@depreciated in favor of drush_log
function _log($message){
    (DEBUG_ON) ? drush_print($message) : '';
}

//builds distribution packageName and fullPath
function get_distribution($name){
    $config = get_config();
    $artifact = get_artifact_by_name($name);

    $artifact_names = artifact_names($name,$artifact['type']);
    $package_name = array_shift($artifact_names);
    $base_name = (empty($artifact_names)) ? $package_name : array_pop($artifact_names);
    $dist_dir = $config['dist'];
    $type = ($artifact['type'] != 'theme') ? 'module' : $artifact['type'] ;
    $artifact_dist = $dist_dir.'/' . $type . 's/';//distitribution dir for the particular artifact
    $full_path = $artifact_dist.$base_name;
    return array(
        'name' => $package_name,
        'path' => $full_path,
//        'artifact_dist' => $artifact_dist,
    );
}

//executes command
function execute_command($command){
//    (DEBUG_ON) ? _log('COMMAND: ' . $command) : '';
    drush_log(dt('COMMAND: @command',array('@command' => $command)), 'debug');
    drush_shell_exec($command);
    if(!preg_match('/zip/',$command)){
        drush_log(dt('@output',array('@output' => implode(',', drush_shell_exec_output()))), 'debug');
//        drush_log(drush_shell_exec_output(), 'debug');
    }
}

//creates versions of artifact full names
function artifact_names($name, $type){
    $config = get_config();
    $alternatives = array();
    $base = $config['package_prefix'].$type.'-'.$name;
    $alternatives[] = $base;

    foreach(get_options() as $key => $option){
        $param = drush_get_option($option, 'none');
        if($param != 'none' || $param == 1){
            if($option == 'min'){
                $param = 'min';
            }
            $base .= '-'.$param;
            $alternatives[] = $base;
        }
    }

    //order more specific first
    $final = array_reverse($alternatives);
//    if(DEBUG_ON){
//        _log('Potential Artifact Names: ');
//        print_r($final);
//    }
    drush_log(dt("Potential Artifact Names: @final", array("@final" => implode(', ', $final))), 'notice');
    return $final;
}

//list of configuration options used to run builds, deployment,etc
function get_options(){
    return array(
        'branch',
        'min',
        'ver',
    );
}

/*
 * executes function by name $action + '_action'
 * @param: $action
 *      an array containing info about action such as the name of function and parameters
 *          -name: name of the action
 *          -param: array of parameters
 *
 * @return:
 *      return from the action
 */
function apply_action($action){

    $params = (isset($action['param']) && !empty($action['param'])) ? $action['param'] : array() ;
    $all_commands = drush_get_commands();
    $drush_command = $all_commands[$action['name']];
    $func = $action['name'] . "_action";
    if (function_exists($func)) {
        drush_log('executing action - '.$func,'notice');
        //action
        return call_user_func_array($func, $params);
    } elseif(isset($drush_command) && !empty($drush_command)){
        //drush command
        if($alias = drush_get_context('FLOW_WIDGET_ALIAS',FALSE)){
            return drush_invoke_process($alias, $action['name'], $params);
        }else{
            return drush_dispatch($drush_command, $params);
        }
    }else {
        drush_log("ERROR. " . $func . " does not exist", 'notice');
    }
}

/*
 * goes over the artifacts and returns the module name
 * @param: $widget_modules
 *      array of artifacts with artifact name
 *
 * @return:$module_names
 *      array of module name for each artifact
 */
function get_widget_module_names($widget_modules){
    $module_names = array();
    foreach($widget_modules as $key => $artifact_name){
        $artifact = get_artifact_by_name($artifact_name);
        if(isset($artifact['module_name']) && !empty($artifact['module_name'])){
            $module_names[] = $artifact['module_name'];
        }
    }
    return $module_names;
}

function compare_module_names($one, $two){
    if(preg_match('/'.$one.'/',$two)){
        return true;
    }else{
        return false;
    }
}