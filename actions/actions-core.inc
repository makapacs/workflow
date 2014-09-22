<?php
/**
 * Actions Declared
 */

/*Configure Video action configures video by using video library located at MODULE_DIR/libraries/ dir

 * @param: $module_name
 *      name of the module containing vido lib. It is also assumed that the lib is located PATH_TO_MODULE/libraries/player dir
 * @param: $video_player
 *      the name of audio player(i.e. wpaudioplayer). By default, the player is wpaudioplayer
 *
 * [SAMPLE]
 * action[configure_audio][name]=configure_video
 * action[configure_audio][order]=3
 * action[configure_audio][param][module]=MODULE-NAME
 */

function configure_video_action($module_name, $video_player = 'video-js')
{
    //enabling module
    _log('Enabling module - videojs....');
    enable_artifact_action('videojs');

    //Link VideoJs
    _log('Linking videojs....');
    $current_dir = getcwd();
    $player_dest = $current_dir . '/sites/all/libraries/' . $video_player;
    if (!file_exists('sites/all/libraries')) {
        _log('sites/all/libraries directory does not exist...creating one');
        execute_command('sudo mkdir sites/all/libraries');
    }

    $path = drupal_get_path('module', $module_name);
    $payer_src = $current_dir . '/' . $path . '/libraries/video-js';
    _log('linking player ' . $player_dest . ' to path ' . $payer_src);
    link_action($payer_src, $player_dest);
}

/*
* configures editor by default ckeditor. It also uses Imce module for uploading/inserting images
 * @ToDo currently only supports CKEditor but like to have others
 * @param: $module_name
 *      name of the module containing editor library
 *
 * @param: $editor
 *      type of the editor to configure by default ckeditor
 */

function configure_editor_action($module_name, $editor = 'ckeditor')
{
    $alias_record = drush_get_context('WF_ALIAS');
//    define('DRUPAL_ROOT', $alias_record['root']);

    $editor_dest = $alias_record['root'] . '/sites/all/libraries/ckeditor';
    $path = drupal_get_path('module', $module_name);

    //enabling module
    drush_log('enabling module - ' . $module_name . '....', 'ok');
    enable_artifact_action($module_name);

    //enabling module
    drush_log('enabling module - ' . $editor . '....', 'ok');
    enable_artifact_action($editor);

    //enabling module
    drush_log('enabling module - imce....', 'notice');
    enable_artifact_action('imce');

    //link CKeditor
    drush_log('linking ' . $editor . ' library....', 'ok');

    if (!file_exists('sites/all/libraries')) {
        drush_log('sites/all/libraries directory does not exist...creating one', 'notice');
        execute_command('sudo mkdir sites/all/libraries');
    }

    if (file_exists($editor_dest)) {
        drush_log("destination - " . $editor_dest . " already exist....deleting", 'notice');
        execute_command('sudo rm -R ' . $editor_dest);
    }

    $editor_src = $alias_record['root'] . '/' . $path . '/libraries/ckeditor';
    link_action($editor_src, $editor_dest);
}


/*Configure Audio action configures audio player by using player library located at MODULE_DIR/libraries/player
 * @param: name of the module containing audio lib. It also assumed that the lib is located PATH_TO_MODULE/libraries/player dir
 * @param: the name of audio player(i.e. wpaudioplayer). By default, the player is wpaudioplayer
 *
 * @param: $module_name
 *      name of the module
 *
 * @param: $audio_player
 *      name for the audio player to use. By default, the player name is 'wpaudioplayer'
 *
 * [SAMPLE]
 * action[configure_audio][name]=configure_audio
 * action[configure_audio][order]=3
 * action[configure_audio][param][module]=MODULE-NAME
 */

function configure_audio_action($module_name, $audio_player = "wpaudioplayer")
{

    $path = drupal_get_path('module', $module_name);
    //configure audio
    set_action('audiofield_audioplayer', $audio_player);
    $payer_audio_src = $path . "/libraries/player";
    if (!file_exists($payer_audio_src)) {
        drush_die("path to audio player lib does not exist: " . $payer_audio_src, 0);
    }
    variable_set('audiofield_players_dir', $payer_audio_src);
    _log('...configured audio player lib to path ' . $payer_audio_src);
}


/*
 * retrieves blueprint - make file for artifact with name
 *
 * @param: $name
 *      name of the artifact
 * @param: return
 *      full path to the make file
 */
function get_make_file_action($name = NULL)
{
    $name = (isset($name)) ? $name : drush_get_context('WF_ASSET_NAME');
    $make_file = get_make_file($name);
    return $make_file;
}

/*
 * reads all the 'widget' modules and 'project' modules from make file. Then downloads all missing dependencies. Afterwards,
 * it enables all inactive dependencies except modules with prefix as defined in workflow external environment configurations
 *
 */
function configure_all_dependencies_action()
{
    $alias = drush_get_context('WF_ALIAS');
    $make_file = get_make_file_action();

    //grab all dependencies from make file
    $widget_dependencies = get_widget_dependencies_action($make_file);
    $project_dependencies = get_project_dependencies_action($make_file);
    $all_dependent_modules = (isset($project_dependencies) && is_array($project_dependencies)) ? array_unique(array_merge($widget_dependencies, $project_dependencies)) : $widget_dependencies;
    drush_log(dt('ALL DEPENDENT: @modules', array('@modules' => "\n" . implode(',' . "\n", $all_dependent_modules))) . "\n", 'notice');

    //make only the ones disabled or uninstalled to be enabled
    $cur_enabled = array_keys(drush_module_list());
    drush_log(dt('ALL ENABLED: @modules', array('@modules' => "\n" . implode(',' . "\n", $cur_enabled))) . "\n", 'notice');

    //@ToDo make array_udiff execute accordingly...
    //    $all_not_enabled = array_udiff($all_dependent_modules, $cur_enabled, 'compare_module_names');

    //remove artifacts of the package themselves ...those are handled separately
    $declared_artifacts = get_artifacts_from_blueprint('widget');
    $artifact_modules = get_widget_module_names(_array_column($declared_artifacts, 'name'));

    $depenencies_not_enabled = array();
    foreach ($all_dependent_modules as $key => $dependent) {
        if (!in_array(strip_ver($dependent), $cur_enabled) && !in_array($dependent, $artifact_modules)) {
            $depenencies_not_enabled[] = $dependent;
        }
    }

    //filter out workflow modules
    $filtered_inactive_dependencies = array_filter($depenencies_not_enabled, 'not_workflow_module');
    //version map
    $version_map = array(); //for keeping versions for download later
    $striped_dependencies = array_unique(striped_dependencies($filtered_inactive_dependencies, $version_map));

    if (empty($striped_dependencies)) {
        drush_log('all dependencies are already installed & enabled', 'ok');
    } else {
        drush_log(dt('inactive dependencies:' . "\n" . ' @modules', array('@modules' => implode(',' . "\n", $striped_dependencies))), 'ok');

        //enable inactive dependencies
        foreach ($striped_dependencies as $key => $dependent_module) {
            /***Download***/
            //@ToDo fix the bug for 'features_extra' break
            //download first if module is not present or it is with different version than stable
            $target_module = (isset($version_map[$dependent_module])) ? $dependent_module . '-' . $version_map[$dependent_module] : $dependent_module;
//            $name_tokens = explode('-', $target_module);

            //checks to see if module isn't present already or it has been different version than the stable version
            if (!is_module_present(strip_ver($dependent_module)) || $target_module != $dependent_module) {
                drush_log('module ' . $target_module . ((drush_invoke_process($alias, "pm-download", array($target_module)) ? ' WAS ' : ' WAS NOT')) . ' downloaded', 'ok');
            } else {
                drush_log("\n" . 'module - ' . $dependent_module . ' is already present...skip downloading', 'ok');
            }

            /***Enable***/
            //strip version, so it can be enabled
//            $dependent_module = array_shift($name_tokens);
            $target = _fix_dependency($dependent_module);
            if (!module_exists($target)) {
                drush_log('module ' . $target . ((drush_invoke_process($alias, "pm-enable", array($target)) ? ' WAS ' : ' WAS NOT')) . ' enabled', 'ok');
            } else {
                drush_log('module - ' . $target . ' is already enabled...skip enabling', 'ok');
            }

        }
    }
}


/*
 * Reads make file and downloads all the dependencies specified with 'widget'
 *
 * @param $make_file
 *      full path to make_file
 *
 * @return: array
 *      module names declared in make file with 'widget' tag
 */
function download_widget_dependencies_action($make_file)
{
    //download all projects specified in the .make file
    $widget_modules = get_widget_dependencies_action($make_file);
    $alias = drush_get_context('WF_ALIAS');

    foreach ($widget_modules as $key => $module) {
        //download
//        drush_log('about do download ' . $module . "...", 'ok');
        drush_log('module ' . $module . ((drush_invoke_process($alias, "pm-download", array($module)) ? ' WAS ' : ' WAS NOT')) . ' downloaded', 'ok');
    }
}

/**
 * Downloads one or many modules
 * @param $name
 *      the name of module to download one by one
 */
function download_module_action($name)
{
    $args = func_get_args();
    $site_record = drush_get_context('WF_ALIAS', '@self');
    drush_log('module ' . $name . ((drush_invoke_process($site_record, "pm-download", $args) ? ' WAS ' : ' WAS NOT')) . ' downloaded', 'notice');
}


/*
 * Reads make file and downloads all the dependencies specified with 'project'
 *
 * @param: $make_file
 *      full path to make_file
 *
 * @return: array
 *     module names declared in make file with 'project' tag
 */
function dl_project_dependencies_action($make_file = '')
{
    $make = (!empty($make_file)) ? $make_file : get_make_file();
    //download all projects specified in the .make file
    $project_modules = get_project_dependencies_action($make);

    foreach ($project_modules as $key => $module) {
        //download
        if (!module_exists($module)) {
            drush_log('module ' . $module . ((drush_invoke_process("@self", "pm-download", array($module)) ? ' WAS ' : ' WAS NOT')) . ' downloaded', 'ok');
        } else {
            drush_log('module - ' . $module . ' already present...skip downloading', 'notice');
        }
    }
}

/*
 * retrieves project dependencies
 *
 * @param $make_file
 *      full path to make_file
 *
 * @return: array
 *      module names declared in make file with 'project' tag
 */
function get_project_dependencies_action($make_file)
{
    if (!file_exists($make_file)) {
        drush_die("Make File - " . $make_file . " does not exist");
    }
    //download all projects specified in the .make file
    $build_file_parsed = drupal_parse_info_file($make_file);
    $project_modules = _project_modules_from_make($build_file_parsed);

    return $project_modules;
}

/*
 * reads make file for all widgets declared with 'widget' tag. returns the list after removing widget dependent on the another widget
 *
 * @param: $make_file
 *      path to make file
 * @return: array of module names that widgets dependent
 */
function get_widget_dependencies_action($make_file)
{
    if (!file_exists($make_file)) {
        return drush_set_error('WORKFLOW_ACTION_ERROR', dt('Make File - @make_file does not exist', array('@make' => $make_file)));
    }

    $build_file_parsed = drupal_parse_info_file($make_file);

    $widget_modules = _array_column(array_orderby($build_file_parsed['widget'], 'order', SORT_ASC), "name");
    $widget_names = get_widget_module_names($widget_modules);
    $all_dependent_modules = array();

    //retrieving dependency list
    drush_include_engine('drupal', 'environment', drush_drupal_major_version());
    $module_info = drush_get_modules();
    $dependency_status = drush_check_module_dependencies($widget_names, $module_info);
    $dependencies_per_widget = _array_column($dependency_status, 'dependencies');
    foreach ($dependencies_per_widget as $key => $mod_dependancies) {
        $all_dependent_modules = (isset($mod_dependancies) && is_array($mod_dependancies)) ? array_unique(array_merge($all_dependent_modules, $mod_dependancies)) : $all_dependent_modules;
    }

    drush_log(dt('Widget Modules: @modules', array('@modules' => implode(',' . "\n", $widget_modules))), 'notice');
    drush_log(dt('All Dependent MODULES: @modules', array('@modules' => implode(',' . "\n", $all_dependent_modules))), 'notice');

    //remove widget modules for being enabled
    $dependencies_without_widgets = array_diff($all_dependent_modules, $widget_modules);
    return $dependencies_without_widgets;
}


/*
 * Link Action creates symbolic from one location to another
 * @param: $src_dir
 *      source directory
 * @param: $dest_dir
 *      destination directory
 */

function link_action($src_dir, $dest_dir)
{

    if (!file_exists($src_dir)) {
        drush_log("ERROR: source location - " . $src_dir . " does not exist", 'notice');
    }


//    if(file_exists($dest_dir)){
    drush_log("in case destination - " . $dest_dir . " already exist....lets try to delete", 'notice');
    execute_command('sudo rm -R ' . $dest_dir);
//    }

    execute_command('sudo ln -s ' . $src_dir . ' ' . $dest_dir);

//    (symlink($src_dir, $dest_dir)) ? _log('linking '.$src_dir.' to  '.$dest_dir) : _log("***ERROR: didn't linked to ".$dest_dir);
}

/*
 * Enables one or many modules or themes by name on instance specified by alias
 *
 * @param $name
 *      the names of modules to be enabled
 *
 * action[en-context][name] = enable_artifact
 * action[en-context][order] = 1
 * action[en-context][param][] = context_ui
 *
 */
function enable_artifact_action($name)
{
    $args = func_get_args();
    $site_record = drush_get_context('WF_ALIAS', '@self');
    //@ToDo implement check if artifact is not present, then download
    drush_log('artifact ' . $name . ((drush_invoke_process($site_record, "pm-enable", $args) ? ' WAS ' : ' WAS NOT')) . ' enabled', 'notice');
}

function revert_feature_action($name)
{
    $args = func_get_args();
    $site_record = drush_get_context('WF_ALIAS', '@self');
    foreach ($args as $key => $feature) {
        drush_log('artifact ' . $feature . ((drush_invoke_process($site_record, 'features-revert', array($feature))) ? ' WAS ' : ' WAS NOT') . ' reverted', 'notice');
    }
}

/*
 * Sets theme to default one on instance specified by alias
 *
 * @param $theme_name
 *      the name of theme
 * @param $alias
 *      the alias of instance theme to enable
 *
 * action[en_bazar][name] = set_default_theme
 * action[en_bazar][order] = 1
 * action[en_bazar][param][] = bazar
 *
 */
function set_default_theme_action($theme_name, $alias = '@self')
{
    //@ToDo check if theme is not present, then download
//drush_log('module ' . $theme_name . ((drush_invoke_process($alias, "pm-enable", array($module_name)) ? ' WAS ' : ' WAS NOT')) . ' enabled', 'notice');
//drush vset theme_default garland
    $cmd = 'drush vset theme_default ' . $theme_name;
    execute_command($cmd);
}


/*
 * Set variable
 *
 * @param: $name
 *      the name of variable
 * @param: $value
 *      the value of variable
 */
function set_action($name, $value)
{
    variable_set($name, $value);
    drush_log('variable "' . $name . '" was set to "' . $value . '"','ok');
//    _log('variable - ' . $name . ((drush_invoke_process("@self", "vset", array($name,$value)) ? ' WAS ' : ' WAS NOT')) . ' set to '.$value);
}

/*
 * different configurations for tuning site to production. Here is the list
 * 1. Remove Update messages(disable "update" module) to stop security messages
 * 2. Turn off messages
 * 3. @ToDo turn on file caching and aggrigation
 * 4. @ToDo Disable devel related modules - devel_themer, devel, coffee,etc
 * 5. @ToDo create local writable dir and assign to admin/config/media/file-system for "Private File Sys Path", "Tempaorary Dir"
 * 6. @ToDo disable shortcuts module
*/
function production_ready_action()
{
    $alias = wf_var('WF_ALIAS');
    //turn off update module
    drush_log('module - "update" ' . ((drush_invoke_process($alias, "dis", array('update')) ? ' WAS ' : ' WAS NOT')) . ' disabled','ok');

    //turn off messages
    set_action('error_level', 0);

    //regular tune up
    tune_up_action();
}

/**
 * this function does following tune up actions
 *   1. @ToDo disable shortcuts module
 *   2. @ToDo disable editor for comments
 *   3. @ToDo enable admin menu
 */
function tune_up_action(){
    $alias = wf_var('WF_ALIAS');
    //disable shortcuts module
    _log('module - "shortcut" ' . ((drush_invoke_process($alias, "dis", array('shortcut')) ? ' WAS ' : ' WAS NOT')) . ' disabled');
}

/**
 * @param string $ver
 * @param string $compression
 */

/*Enables jquery_update module and sets version
 *
 * @param: $ver
 *      jQuery lib version
 * @param: $compression
 *      set whether compressing the jquery or not
 *
 * */
function configure_jquery_action($ver = "1.10", $compression = "min")
{
    drush_log('configuring jquery_update...','ok');
    enable_artifact_action('jquery_update');
    set_action('jquery_update_compression_type', "min");
    set_action('jquery_update_jquery_cdn', "google");
    set_action('jquery_update_jquery_admin_version', $ver);
    set_action('jquery_update_jquery_version', $ver);
}

/*
 * Enable permissions to the public directory
 */
function permit_public_dir_action($permissions = '777', $dir = 'sites/default/files')
{
    $alias = drush_get_context('WF_ALIAS');
    $pub_dir = (isset($alias) && isset($alias['root'])) ? $alias['root'] . '/sites/default/files' : '';

    execute_command('sudo chmod -R ' . $permissions . ' ' . $pub_dir);
}

/**
 * list info about each artifact
 *
 */
function get_artifact_info_action()
{
    $make = get_make_file();

    if (!file_exists($make)) {
        drush_die(dt('ERROR: the blueprint(make file) not provided'), 0);
    }
    $all_artifacts = get_artifacts_from_blueprint();
    $table = array();
    $header = array('Name', 'Blueprint (ver)', 'Work Dir (ver)', 'Status');
    foreach ($all_artifacts as $key => $artifact) {
        $blueprint_ver = get_artifact_blueprint_ver($artifact['name']);
        $table[] = array($artifact['name'], $artifact['ver'], $blueprint_ver, ($blueprint_ver == $artifact['ver']) ? 'newest' : 'NOT NEWEST');
    }
    array_unshift($table, $header);
    drush_print_table($table, TRUE);
}


/**
 * list artifacts that have been changed but not commited yet in working dir
 * @return
 *      prints artifact names
 */

function list_updated_artifacts_action()
{
    $updated_artifacts = get_uncommitted_artifacts();
    if (empty($updated_artifacts)) {
        drush_log('all artifact\'s working directories are up to date', 'ok');
    } else {
        foreach ($updated_artifacts as $key => $artifact) {
            drush_log($artifact['name'] . ' been updated and is not committed', 'ok');
        }
    }
}

/**
 * update each artifact's working directory in blueprint if anything changed for particular package/artifact
 *
 */
function update_all_work_dir_action($message = 'updated work dir')
{
    $updated_artifacts = get_uncommitted_artifacts();
    foreach ($updated_artifacts as $key => $artifact) {
//        print('name: ' . $artifact['name']);
        update_work_dir_action($artifact['name'], $message);
    }
}

/**
 * updates artifact's working directory if anything changed for particular package/artifact
 *
 */
function update_work_dir_action($artifact_name, $message = 'updated work dir')
{
    $work_dir = get_artifact_work_dir($artifact_name);
    $artifact = get_artifact_by_name($artifact_name);
    $new_ver = increment_blueprint_ver($work_dir);
    chdir($work_dir);
    drush_shell_exec('git add .');
    drush_log('OUTPUT[git add .]:' . implode(',', drush_shell_exec_output()), 'notice');
    drush_shell_exec('git commit -m "' . $message . ' - ' . $new_ver . '"');
    drush_log('OUTPUT[git commit]:' . implode(',', drush_shell_exec_output()), 'notice');
    drush_shell_exec('git tag -a "' . $new_ver . '" -m "update ver:' . $new_ver . '"');
    drush_log('OUTPUT[git tag ]:' . implode(',', drush_shell_exec_output()), 'notice');

    drush_log('updating repo - '.$artifact['repo'].$artifact['repo_name'],'ok');
    $branch = (isset($artifact['branch']) && !empty($artifact['branch'])) ? $artifact['branch'] : 'master';
    drush_shell_exec('git push origin ' . $branch);
    drush_log('OUTPUT[git push origin]:' . implode(',', drush_shell_exec_output()), 'notice');
    drush_shell_exec('git push --tags "');
    drush_log('OUTPUT[git push tags ]:' . implode(',', drush_shell_exec_output()), 'notice');

    drush_log('artifact - ' . $artifact_name . ' was updated to ver: ' . $new_ver, 'ok');
}

/**
 * goes to the artifact working dir and grabs uncommitted differences to display
 * @param: $artifact_name
 *      artifact name to which working dir it will go
 */
function see_diff_action($artifact_name)
{
    $work_dir = get_artifact_work_dir($artifact_name);
    drush_log('looking into work dir - ' . $work_dir, 'notice');
    chdir($work_dir);
    drush_shell_exec('git diff');
    $diff = drush_shell_exec_output();
    drush_shell_exec('git diff --staged');
    $output = array_merge($diff, drush_shell_exec_output());
    print_r($output);
}

/**
 * Clear all action clears all cash and runs cron jobs
 */
function clear_all_action()
{
    if ($alias = wf_var('WF_ALIAS', FALSE)) {
        drush_log('clearing all for ' . $alias['root'], 'ok');
        chdir($alias['root']);
        drush_shell_exec('drush cc all');
        $diff = drush_shell_exec_output();
        drush_log('ouput for clearing cache: ' . "\n" . implode(',' . "\n", $diff), 'notice');
        drush_shell_exec('drush cron');
        $output = array_merge($diff, drush_shell_exec_output());
        drush_log('ouput for clearing cache: ' . "\n" . implode(',' . "\n", $output), 'notice');
    } else {
        drush_log('ERROR: alias not specified..', 'ok');
    }

}

/**
 * creates new artifact(i.e.widget,theme) boiler plate
 * @param: $name
 *      the name of the new widget
 * @param: $type
 *      the type of widget(i.e widget,theme)
 *
 */

function create_boilerplate_action($artifact_name)
{
    $artifact = get_artifact_by_name($artifact_name);
    if(isset($artifact) && !empty($artifact)){
        $type = $artifact['type'];
        $name = $artifact['name'];
        //create working directory for new artifact
        $dir = get_work_dir($artifact);
        execute_command('sudo mkdir ' . $dir);
        drush_log('working directory for artifact ' . $name . ' was created at ' . $dir, 'ok');

        //update permission to write
        execute_command('sudo chmod -R 777 ' . $dir);

        //write boilerplate
        write_boilerplate($artifact_name);
        drush_log('boilerplate written for ' . $type, 'ok');
    } else {
        drush_log('ERROR: artifact not found by name - ' . $artifact_name, 'ok');
    }


}

/**
 * creates widget boilerplate
 *      -creates working directory
 *      -write boilerplate info file and others
 *      -initiates repo and pushes first commit
 * $param: $artifact_name
 *      name of artifact as declared in the artifact configurations
 */

function create_artifact_action($artifact_name)
{
    //creates boiler plate
    create_boilerplate_action($artifact_name);
    //hooks git repo
    hook_git_repo($artifact_name);
}
