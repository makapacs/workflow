<?php

/******COMMON TASKS SHARED BETWEEN SCRIPTS*******/

//Check to see if we should back things up first.
if (drush_get_option('backup-files')) {
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
        print_log('options not set..', 'notice');
        $options = array();
        $options['debug'] = drush_get_option('debug', FALSE);
        $options['no-core'] = drush_get_option('no-core', FALSE);
        $options['yes'] = TRUE;
        ($contr_dest = drush_get_option('contrib-destination', FALSE)) ? $options['contrib-destination'] = $contr_dest : '';
    }
    print("Debug - OPTIONS:");
    print_r($options);

    if ($dest) {
        drush_log('make_file: ' . $make_file . ' dest: ' . $dest . ' optionss:' . implode(',', $options), 'notice');
        drush_log("SITE INSTALL", 'ok');
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

function build_artifact($artifact, $dest)
{
    $artifact_config = get_artifact_by_name($artifact['name']);
    //artifact name in the makefile is different than one in artifact configuration or Artifact Config cache is not set.
    if (!isset($artifact_config) || empty($artifact_config)) {
        drush_log('ERROR: artifact name mismatch - or - artifact cache not set', 'notice');
        $artifact_config = $artifact;
    }

    if (empty($artifact_config['repo']) || empty($artifact_config['repo_name']) || empty($artifact_config['branch'])) {
        drush_die('Artifact - ' . $artifact['name'] . ' is not complete....unable to build', 0);
    }
    //keep it consistent with drupal naming and file structure
    $type = (isset($artifact_config['type']) && $artifact_config['type'] == 'widget') ? 'module' : $artifact_config['type'];
//    $type = $artifact_config['type'];

    $widget_dist_dest_lib = $dest . '/' . $type . 's/' . $artifact_config['final_name'];
    //@ToDo make it clone the tag as well to avoid separate co for tag
    $command = 'sudo git clone -b ' . $artifact_config['branch'] . ' ' . $artifact_config['repo'] . $artifact_config['repo_name'] . '.git ' . $widget_dist_dest_lib;
    drush_log(dt('cloning ...@repo', array('@repo' => $artifact_config['repo'])), 'ok');
    execute_command($command);

    if (isset($artifact_config['ver']) && !empty($artifact_config['ver'])) {
        $command = 'sudo git checkout ' . $artifact_config['ver'];
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

//@Deprecated
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
    $widget_name = drush_get_context('WF_ASSET_NAME');

    //list of artifacts in the package
    $artifacts = array();

    //add dependent artifacts
    //read make file
    //retrieving artifacts to be included in the build
    $make_file = get_make_file($widget_name);

    $build_file_parsed = drupal_parse_info_file($make_file);

    $artifacts_configured = array();
    if ($type != 'all') {
        $artifacts_configured = (isset($build_file_parsed[$type])) ? array_orderby($build_file_parsed[$type], 'order', SORT_ASC) : array();
    } else {
        foreach (_all_artifact_types() as $key => $config_type) {
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

/*
 * retrieves all actions from all make files in the directory
 * @param: $dir
 *      directory name with make files
 *
 * @return: array of actions from all make files in the directory
 */
function get_actions_from_dir($dir)
{
    $actions = array();
    return $actions;
    //read all make files and
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
function get_make_file($name = '')
{
    if ($make = drush_get_context('FW_CACHED_MAKE', FALSE)) {
        drush_log('using make file from cache...' . $make, 'notice');
        return $make;
    } else {
        //make file provided via --make
        if (!file_exists($make_file = wf_var('WF_ASSET_MAKE'))) {
            if (!file_exists($make_file = wf_var('WF_ASSET_MAKE'))) {
                //check if relative path provided
                $make_file = wf_var('WF_MAKE_ROOT') . '/' . wf_var('WF_ASSET_MAKE');
            }
        }

        $temp_make = wf_var('WF_ASSET_MAKE', '');
        if (!file_exists($make_file) || empty($temp_make)) {
            //see if make file provided via --package
            $package = wf_var('WF_ASSET_PACKAGE', '');
            $package_dir = (file_exists($package)) ? $package : wf_var('WF_DIST') . '/' . $package;
            if (!empty($package) && file_exists($package_dir)) {
                //read all make files from package directory
                foreach (glob($package_dir . "/*.make") as $filename) {
                    $make_file = $filename;
                }
            } elseif (empty($temp_make) || !file_exists($make_file)) {
                $temp_make = wf_var('WF_ASSET_MAKE', '');
                if (!file_exists($make_file) || empty($temp_make)) {
                    //try read the make specified by artifact
                    $name = (!empty($name)) ? $name : wf_var('WF_ASSET_NAME', 'no-artifact');
                    $artifact = get_artifact_by_name($name);

                    if (isset($artifact)) {
                        $list_of_names = (isset($artifact['type'])) ? artifact_names($name, $artifact['type']) : artifact_names($name, 'widget');
                        foreach ($list_of_names as $key => $artifact_name) {
                            $make_file = wf_var('WF_MAKE_ROOT') . '/';
                            $make_file .= (isset($artifact['type'])) ? $artifact['type'] . 's/' : '';
                            $make_file .= $artifact_name . '.make';

                            if (file_exists($make_file)) {
                                drush_log('make_file found ' . $make_file, 'ok');
                                drush_log('caching make file...' . $make_file, 'notice');
                                drush_set_context('FW_CACHED_MAKE', $make_file);
                                return $make_file;
                            }
                        }
                    }

                    $default_make = get_default_make_file();
                    drush_log(dt('Artifact blueprint(make file) was not found. Using default - ' . $default_make), 'ok');
                    drush_log('caching make file...' . $default_make, 'notice');
                    drush_set_context('FW_CACHED_MAKE', $default_make);
                    return $default_make;
                }
            }
        }

        //ensure full path
        $make_file = realpath($make_file);
        drush_log('blueprint(make file) found ' . $make_file, 'notice');
        drush_log('caching make file...' . $make_file, 'notice');
        drush_set_context('FW_CACHED_MAKE', $make_file);
        return $make_file;
    }

}

function remove_cur_artifact($name, $type)
{
    $alias = drush_get_context('WF_ALIAS');
    $root = $alias['root'];
    $artifact = get_artifact_by_name($name);

    $type = ($type != 'theme') ? 'module' : $type;
    if (isset($artifact) && !empty($artifact) && isset($artifact['final_name']) && !empty($artifact['final_name'])) {
        $dest_dir = $root . '/sites/all/' . $type . 's/' . $artifact['final_name'];

//    else {
//        //we may be quering the artifact by final name instead the actual name when reading from current drupal instance
//        $dest_dir = $root . '/sites/all/' . $type . 's/' . $name;
//    }
//    drush_log('remove_cur_artifiact checks destination: '.$dest_dir, 'notice');
        if (file_exists($dest_dir)) {
            drush_log('deleting existing ' . $type . ': ' . $name . '....' . $dest_dir, 'notice');
            execute_command('sudo rm -R ' . $dest_dir);
        } else {
            drush_log('ERROR: artifact - ' . $dest_dir . ' to be removed doesn\'t exist', 'notice');
        }
    }else{
        drush_log('ERROR: unable destination of artifact - '.$name, 'notice');

    }
}

function get_artifact_by_name($name)
{


    //for masking out defautl artifact conigurations over the ones in the make file
//    $artifacts_configured = get_artifacts_from_blueprint();
//    $artifact = $artifacts_configured['name'];

    if ($art = drush_get_context('WF_ARTIFACTS', FALSE)) {
        drush_log('artifact declarations masked out ', 'debug');
        return ($art && isset($art[$name])) ? $art[$name] : array();
    } else {
        return drush_set_error('DRUSH_ARTIFACT_INIT_FAILED', dt('WF_ARTIFACTS are not set'));
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
    $artifact_types = ($types = drush_get_context('WF_ARTIFACTS_TYPES', FALSE)) ? $types : array('widget', 'theme');
    return $artifact_types;
}

//removing artifacts based on whats in package
function remove_package_artifacts($dist_package, $type)
{
    //removing artifacts based on whats in the package
    $type = ($type != 'theme') ? 'module' : $type;
    $handle = opendir($dist_package . '/' . $type . 's/');
    while ($handle && false !== ($module = readdir($handle))) {
        if ($module !== '.' && $module !== '..') {
            drush_log('deleting...' . $module, 'notice');
            remove_cur_artifact($module, $type);
        }
    }
    closedir($handle);
}

//removes distribution usually when rebuilding a new one
function remove_distribution($package)
{
//    (DEBUG_ON) ? drush_print('Build: ' . $package['name'] . ' exists...deleting') : '';
    drush_log(dt('build - @package exists...deleting', array('@package' => $package['name'])), 'notice');
    execute_command('sudo rm -R ' . $package['path'] . '/' . $package['name']);
//    drush_shell_exec('sudo rm -R ' . $package['path'].'/'.$package['name']);
//    drush_log(dt('output: @output',array('@output' => implode(',',drush_shell_exec_output()))),'debug');
//    (DEBUG_ON) ? print_r(drush_shell_exec_output()) : '';

    //removing old zip file as well
    execute_command('sudo rm ' . $package['path'] . '/' . $package['name'] . '.tar.gz');
//    drush_shell_exec('sudo rm ' . $package['path'].'/'.$package['name'] . '.tar.gz');
//    (DEBUG_ON) ? print_r(drush_shell_exec_output()) : '';

    //removing old zip file as well
    execute_command('sudo rm ' . $package['path'] . '/' . $package['name'] . '.zip');
//    drush_shell_exec('sudo rm ' . $package['path'].'/'.$package['name'] . '.zip');
//    (DEBUG_ON) ? print_r(drush_shell_exec_output()) : '';
}

//removing artifacts based on whats in the make file
function remove_make_artifacts($type)
{
    foreach (get_artifacts_from_blueprint($type) as $key => $artifact) {
        drush_log(dt('Removing From Make File...artifact_name:' . $artifact['name']), 'notice');
        remove_cur_artifact($artifact['name'], $type);
    }
}

//@depreciated in favor of drush_log
function _log($message)
{
    (DEBUG_ON) ? drush_print($message) : '';
}

//builds distribution packageName and fullPath
function get_distribution($name)
{
    $artifact = get_artifact_by_name($name);

    $artifact_names = artifact_names($name, $artifact['type']);
    $package_name = array_shift($artifact_names);
    $base_name = (empty($artifact_names)) ? $package_name : array_pop($artifact_names);
    $dist_dir = wf_var('WF_DIST');
//    $type = ($artifact['type'] != 'theme') ? 'module' : $artifact['type'];
    $type = $artifact['type'];
    $artifact_dist = $dist_dir . '/' . $type . 's/'; //distribution dir for the particular artifact
    $full_path = $artifact_dist . $base_name;
    return array(
        'name' => $package_name,
        'path' => $full_path,
//        'artifact_dist' => $artifact_dist,
    );
}

//executes command
function execute_command($command)
{
//    (DEBUG_ON) ? _log('COMMAND: ' . $command) : '';
    drush_log(dt('COMMAND: @command', array('@command' => $command)), 'notice');
    drush_shell_exec($command);
    if (!preg_match('/zip/', $command)) {
        drush_log(dt('@output', array('@output' => implode(',', drush_shell_exec_output()))), 'notice');
//        drush_log(drush_shell_exec_output(), 'debug');
    }
}

//creates versions of artifact full names
function artifact_names($name, $type)
{
    $alternatives = array();
    $base = wf_var('WF_PACKAGE_PREFIX') . $type . '-' . $name;
    $alternatives[] = $base;

    foreach (get_options() as $key => $option) {
        $param = drush_get_option($option, 'none');
        if ($param != 'none' || $param == 1) {
            if ($option == 'min') {
                $param = 'min';
            }
            $base .= '-' . $param;
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
function get_options()
{
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
function apply_action($action)
{
    $params = (isset($action['param']) && !empty($action['param'])) ? $action['param'] : array();
    $all_commands = drush_get_commands();
    $drush_command = (isset($action['name']) && isset($all_commands[$action['name']])) ? $all_commands[$action['name']] : null;
    $func = $action['name'] . "_action";
    if (function_exists($func)) {
        drush_log(dt('executing action - @func with arguments: @params', array(
            '@func' => $func,
            '@params' => implode(',' . "\n", $params),
        )), 'notice');
        //action
        return call_user_func_array($func, $params);
    } elseif (isset($drush_command) && !empty($drush_command)) {
        //drush command
        if ($alias = drush_get_context('WF_ALIAS', FALSE)) {
            drush_log(dt('executing drush command - @command with arguments: @params and options: @options',
                array(
                    '@command' => $action['name'],
                    '@params' => "\n" . implode(',' . "\n", $params) . "\n",
                    '@options' => "\n" . implode(',' . "\n", $action['option']) . "\n"
                )
            ), 'notice');

            //to avoid error -Unknown options, we add tag - strict
            $action['option']['strict'] = 0;
            return drush_invoke_process($alias, $action['name'], $params, $action['option']);
        } else {
            drush_log(dt('dispatching drush command - @command with arguments: @params',
                array(
                    '@command' => $action['name'],
                    '@params' => implode(',' . "\n", $params),
                )
            ), 'notice');
            return drush_dispatch($drush_command, $params);
        }
    } else {
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
function get_widget_module_names($widget_modules)
{
    $module_names = array();
    foreach ($widget_modules as $key => $artifact_name) {
        $artifact = get_artifact_by_name($artifact_name);
        if (isset($artifact['module_name']) && !empty($artifact['module_name'])) {
            $module_names[] = $artifact['module_name'];
        }
    }
    return $module_names;
}

function compare_module_names($one, $two)
{
    $first = strip_ver($one);
    $second = strip_ver($two);
    return strcmp($first, $second);
}

function strip_ver($module_name)
{
    if ($pos = strpos($module_name, '-')) {
        return substr($module_name, 0, $pos);
    } else {
        return $module_name;
    }
}

/**@ToDo this will break for files larger 4096...need a fix
 * appends part of one file to another starting with $start_token and ending $end_token
 * @param $source
 *     the path to file the content is read
 * @param $dest
 *     the path to file the content is written to
 * @param $start_token
 *     the string token from the where to read file
 * @param $end_token
 *     the string token of the end reading file
 *
 */

function append_content($source, $dest, $start_token, $end_token)
{
    //    $actions = get_actions_from_make(drush_get_context('WF_DEFAULT_MAKE'));
    $handle_default = fopen($source, 'r');
    $all_make_content = fread($handle_default, 4096);

    $start_pos = mb_strpos($all_make_content, $start_token) - 1;
    $end_pos = mb_strpos($all_make_content, $end_token);

    $size_to_read = strlen(substr($all_make_content, $start_pos, $end_pos));
    $star_offset = strlen(substr($all_make_content, 0, $start_pos));
    rewind($handle_default);
    fseek($handle_default, $star_offset);
    $action_snap = fread($handle_default, $size_to_read);

    if ($ret = file_put_contents($dest, "\n" . $action_snap, FILE_APPEND)) {
        drush_log('appendned ' . $ret . ' bytes to ' . $dest, 'notice');
    } else {
        drush_log('FAILED to append default', 'notice');
    }
}

/**
 * retrieves package name form the full path
 * @param $full_path
 */

function get_package_name($package_temp_dest)
{
    $path_tokens = explode('/', $package_temp_dest);
    $last_token = array_pop($path_tokens);
    //removed extension if exists
    $raw_name = (strpos($last_token, '.tar.gz')) ? substr($last_token, 0, -7) : substr($last_token, 0, -4);
    return $raw_name;
}

/**
 * apply any fix to the modules before downloaded,enabled
 * @param: $module_name
 *      name of the module
 */

function _fix_dependency(&$module_name)
{
    $fixed_module = $module_name;
    if ($module_name == 'features_extra') {
        //fix module name to avoid infinite loop
        $fixed_module = 'fe_block';
    }
    return $fixed_module;
}

/**
 * checks if module is with the prefix defined in the external environment configurations
 * @param:
 */
function not_workflow_module($module)
{
    $patter = str_replace('-', '_', drush_get_context('WF_PACKAGE_PREFIX'));
    if (0 == preg_match('/' . $patter . '/', $module)) {
        return true;
    } else {
        return false;
    }
}

/**
 * checks if module is already downloaded
 * @param $module
 *      name of the module
 * @return bool
 *
 */
function is_module_present($module)
{
    $path_to_module = drupal_get_path('module', $module);
    if (file_exists($path_to_module) && $module != 'features_extra') {
        return true;
    } else {
        return false;
    }
}

/**
 * retrieves working directory for artifact
 * @param: $artifact_name
 *         the name of artifact
 * @return: path to the working directory of the artifact
 */

function get_artifact_work_dir($name)
{
    $artifact = get_artifact_by_name($name);
    $work_dir = (isset($artifact['work_dir']) && !empty($artifact['work_dir'])) ? $artifact['work_dir'] : drush_get_context('WF_WORK_DIR');
    $work_dir_name = (isset($artifact['work_dir_name']) && !empty($artifact['work_dir_name'])) ? $artifact['work_dir_name'] : $name;

    $potential_work_dir = $work_dir . '/' . $artifact['type'] . 's/' . $work_dir_name;
    if (!file_exists($potential_work_dir)) {
        $potential_work_dir = $work_dir . '/' . $work_dir_name;
    }
    return $potential_work_dir;
}

/**
 * retrieves artifacts that have uncommitted changes in their working dir
 *
 * @return
 *      array of type - artifacts that is in uncommitted state
 */
function get_uncommitted_artifacts()
{
    $updated_artifacts = array();
    $make = get_make_file();

    if (!file_exists($make)) {
        drush_die(dt('ERROR: the blueprint(make file) not provided'), 0);
    }

    $all_artifacts = get_artifacts_from_blueprint();
    foreach ($all_artifacts as $key => $artifact) {
        $work_dir = get_artifact_work_dir($artifact['name']);
        chdir($work_dir);
        drush_shell_exec('git status');
        $status = drush_shell_exec_output();
        if (isset($status[1]) && $status[1] != 'nothing to commit, working directory clean') {
            $updated_artifacts[] = $artifact;
        }
    }

    return $updated_artifacts;
}

/**retrieves the version as listed in the artifacts blueprint
 * @param $artifact_name
 *      name of the artifact
 * @return
 *      nubmer of version(i.e.v0.1.2)
 */
function get_artifact_blueprint_ver($artifact_name)
{
    $work_dir = get_artifact_work_dir($artifact_name);
    $info_file = '';
    chdir($work_dir);
    foreach (glob($work_dir . "/*.info") as $filename) {
        $info_file = $filename;
    }
    drush_log('found info file: ' . $info_file . ' in dir ' . $work_dir, 'notice');
    $file = file($info_file);
    if ($file) {
        foreach ($file as $key => $line) {
            if (preg_match('/version/', $line)) {
                $new_ver = _get_ver($line);
                return $new_ver;
            }
        }
    } else {
        drush_log('.info file not found', 'ok');
    }
    return null;
}

/**
 * looks up the .info file, reads version and updates it by one
 * @param $work_dir
 *     directory with tine .info file update version
 * @return number
 *      new version nubmer
 */
function increment_blueprint_ver($work_dir)
{
    $info_file = '';
    $new_ver = '';
    chdir($work_dir);
    foreach (glob($work_dir . "/*.info") as $filename) {
        $info_file = $filename;
    }
    drush_log('found info file: ' . $info_file . ' in dir ' . $work_dir, 'notice');
    $handle_temp = @fopen($work_dir . '/temp.info', "w");
    $file = file($info_file);
    if ($file) {
        foreach ($file as $key => $line) {
            if (preg_match('/version/', $line)) {
                $new_ver = _ver_up($line);
                $line_to_write = 'version = ' . $new_ver . "\n";
                fwrite($handle_temp, $line_to_write, strlen($line_to_write));
            } else {
                fwrite($handle_temp, $line, strlen($line));
            }

        }

        fclose($handle_temp);
    }

    rename($work_dir . '/temp.info', $info_file);
    return $new_ver;
}

//scraps version out and increments
function _ver_up($line)
{
    $only_ver = _get_ver($line);
    $tokens = explode('.', trim($only_ver));
    $last_dig = array_pop($tokens);
    $last_dig = $last_dig + 1;
    $tokens[] = $last_dig;
    return implode('.', $tokens);
}

//scraps version out of line
function _get_ver($line)
{
    return trim(substr($line, strpos($line, '=') + 1));
}

/*
 * wrapper around drush_get_context
 * @param: $var_name
 *      variable name
 * @param: $default
 *      default value
 *
 * return value if set or default value
 */

function wf_var($var_name, $default = false)
{
    $var = drush_get_context($var_name);

    if (!isset($var) || empty($var)) {
        return $default;
    } else {
        return $var;
    }
}

/**
 * strips off all version that are save in $version_map and returns list of modules without duplicates
 *
 */
function striped_dependencies($modules, &$version_map)
{
    foreach ($modules as $key => &$module) {
        if ($pos = strpos($module, '-')) {
            $ver = substr($module, $pos + 1);
            $module = substr($module, 0, $pos);
            $version_map[$module] = $ver;
        }
    }
    return array_unique($modules);
}


/**
 * writes the necessary boilerplate for artifact depending on type
 * @param $artifact_name
 *      the name of artifact
 */
function write_boilerplate($artifact_name)
{
    $artifact = get_artifact_by_name($artifact_name);
    $dest = get_work_dir($artifact);
    $name = $artifact['name'];
    $module_name = $artifact['module_name'];
    $type = $artifact['type'];
    switch ($type) {
        case 'widget':
            $line_to_write = 'name = DesignsSquare.com ' . strtoupper($name) . ' ' . $type . "\n";
            $line_to_write .= 'description = functionality for Drupal ' . strtoupper($name) . ' ' . $type . "\n";
            $line_to_write .= 'core = 7.x' . "\n";
            $line_to_write .= 'version = v0.0.0' . "\n";
            $line_to_write .= 'project = ' . $module_name . "\n";
            $line_to_write .= 'package = ' . str_replace('-', '.', wf_var('WF_PACKAGE_PREFIX', 'designssquare_com_')) . $type . '.' . $name . "\n";
            //write info file
            $handle_info = fopen($dest . '/' . str_replace('-', '_', wf_var('WF_PACKAGE_PREFIX', 'designssquare_com_')) . $name . '.info', "w");
            fwrite($handle_info, $line_to_write, strlen($line_to_write));
            fclose($handle_info);

            //create module file necessary for module to show up in module list
            $handle_module = fopen($dest . '/' . str_replace('-', '_', wf_var('WF_PACKAGE_PREFIX', 'designssquare_com_')) . $name . '.module', "w");
            fwrite($handle_module, '<?php' . "/n", strlen($line_to_write));
            fclose($handle_module);
            break;
        case 'theme':
            $line_to_write = 'name = ' . $name . "\n";
            $line_to_write .= 'description = Drupal Theme - ' . strtoupper($name) . "\n";
            $line_to_write .= 'version = v0.0.0' . "\n";
            $line_to_write .= 'core = 7.x' . "\n";
            //write info file
            $handle_info = @fopen($dest . '/' . $name . '.info', "w");
            fwrite($handle_info, $line_to_write, strlen($line_to_write));
            fclose($handle_info);

            //create template php file
            $handle_template = @fopen($dest . '/template.php', "w");
            fwrite($handle_template, '<?php', strlen($line_to_write));
            fclose($handle_template);
            break;
    }
}


/**
 * initiates git repo, adds repository and push for the first time
 * @param: $artifact_name
 *      the name of artifact as declared in the artifacts.drushrc.php
 */

function hook_git_repo($artifact_name)
{
    $artifact = get_artifact_by_name($artifact_name);
    $dest = get_work_dir($artifact);
    $git_repo = get_artifact_repo($artifact);
    if (file_exists($dest)) {
        chdir($dest);
        execute_command('git init');
        execute_command('git add .');
        execute_command('sudo chmod -R 777 .git');
        execute_command('git tag -a "v0.0.0" -m "init commit"');
        execute_command('git commit -m "first commit"');

        if (!empty($git_repo)) {
            execute_command('git remote add origin ' . $git_repo);
        } else {
            drush_log('ERROR: git repo not specified', 'notice');
        }
        drush_log('...pushing into repo - ' . $git_repo, 'ok');
        execute_command('git push origin master');
        execute_command('git push --tags');
        drush_log('finished configuring git repo - ' . $git_repo . ' in the working directory ' . $dest, 'notice');
    } else {
        drush_log('ERROR:  dest - ' . $dest . ' specified does not exist', 'notice');
    }
}


/**
 * builds path to working directory from artifact provided
 * @param: $artifact
 *      array of artifact
 * @return:
 *      full path to working directory of the artifact
 */
function get_work_dir($artifact)
{
    if (isset($artifact) && !empty($artifact)) {
        $dir_name = (isset($artifact['work_dir_name']) && !empty($artifact['work_dir_name'])) ? $artifact['work_dir_name'] : $artifact['name'];
        $dir_path = (isset($artifact['work_dir']) && !empty($artifact['work_dir'])) ? $artifact['work_dir'] : wf_var('WF_WORK_DIR') . '/' . $artifact['type'] . 's';
        $dir = $dir_path . '/' . $dir_name;
        return $dir;
    } else {
        drush_log('ERROR: artifact not specified', 'notice');
        return '';
    }
}

/**
 * builds path to the repo from the artifact provided
 * @param: $artifact
 *      array of artifact
 * @return
 *      full path of git repo
 */
function get_artifact_repo($artifact)
{
    if (isset($artifact) && !empty($artifact)) {
        return $artifact['repo'] . $artifact['repo_name'] . '.git';
    } else {
        drush_log('ERROR: artifact not specified', 'notice');
        return '';
    }

}