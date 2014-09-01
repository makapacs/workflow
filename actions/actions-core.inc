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
    $alias_record = drush_get_context('FLOW_WIDGET_ALIAS');
    define('DRUPAL_ROOT', $alias_record['root']);

    $editor_dest = $alias_record['root'] . '/sites/all/libraries/ckeditor';
    $path = drupal_get_path('module', $module_name);

    //enabling module
    drush_log('enabling module - ' . $module_name . '....', 'notice');
    enable_artifact_action($module_name);

    //enabling module
    drush_log('enabling module - ' . $editor . '....', 'notice');
    enable_artifact_action($editor);

    //enabling module
    drush_log('enabling module - imce....', 'notice');
    enable_artifact_action('imce');

    //link CKeditor
    drush_log('linking ' . $editor . ' library....', 'notice');

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
    $name = (isset($name)) ? $name : drush_get_context('FLOW_WIDGET_NAME');
    $make_file = get_make_file($name);
    return $make_file;
}

/*
 * reads all the 'widget' modules and 'project' modules from make file. Then downloads all missing dependencies. Afterwards,
 * it enables all the dependencies except the ones specified as 'widgets' in the make file
 *
 */
function configure_all_dependencies_action()
{
    $make_file = get_make_file_action();
    $alias = drush_get_context('FLOW_WIDGET_ALIAS');

    //grab all dependencies from make file
    $widget_dependencies = get_widget_dependencies_action($make_file);
    $project_dependencies = get_project_dependencies_action($make_file);
    $all_dependent_modules = (isset($project_dependencies) && is_array($project_dependencies)) ? array_unique(array_merge($widget_dependencies, $project_dependencies)) : $widget_dependencies;
    //make only the ones disabled or uninstalled to be enabled
    $cur_enabled = array_keys(drush_module_list());
//    $all_not_enabled = array_diff($all_dependent_modules, $cur_enabled);
    $all_not_enabled = array_udiff($all_dependent_modules, $cur_enabled, 'compare_module_names');
    $depenencies_not_enabled = array_intersect($all_dependent_modules, $all_not_enabled);

    if(empty($depenencies_not_enabled)){
        drush_log('all dependencies are already installed & enabled','ok');
    }else{
        drush_log(dt('inactive dependencies:'."\n".' @modules', array('@modules' => implode(',' . "\n", $depenencies_not_enabled))), 'ok');
    }

    //enable inactive dependencies
    foreach ($depenencies_not_enabled as $key => $dependent_module) {

        //@ToDo fix the bug for 'features_extra' break
        if ($dependent_module != 'features_extra') {
            //download first if module with different version than stable
            $name_tockens = explode('-', $dependent_module);
            if (count($name_tockens) > 1) {
                //
                drush_log('module ' . $dependent_module . ((drush_invoke_process($alias, "pm-download", array($dependent_module)) ? ' WAS ' : ' WAS NOT')) . ' downloaded', 'ok');
                //strip version, so it can be enabled
                $dependent_module = array_shift($name_tockens);
            }

            drush_log('module ' . $dependent_module . ((drush_invoke_process($alias, "pm-enable", array($dependent_module)) ? ' WAS ' : ' WAS NOT')) . ' enabled', 'ok');
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
    $alias = drush_get_context('FLOW_WIDGET_ALIAS');

    foreach ($widget_modules as $key => $module) {
        //download
//        drush_log('about do download ' . $module . "...", 'ok');
        drush_log('module ' . $module . ((drush_invoke_process($alias, "pm-download", array($module)) ? ' WAS ' : ' WAS NOT')) . ' downloaded', 'ok');
    }
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
function download_project_dependencies_action($make_file)
{
    //download all projects specified in the .make file
    $project_modules = get_project_dependencies_action($make_file);

    foreach ($project_modules as $key => $module) {
        //download
        drush_log('about do download ' . $module . "...", 'notice');
        drush_log('module ' . $module . ((drush_invoke_process("@self", "pm-download", array($module)) ? ' WAS ' : ' WAS NOT')) . ' downloaded', 'ok');
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
 * Enables module or theme by name on instance specified by alias
 *
 * @param $name
 *      the name of module
 * @param $alias
 *      the alias of instance module to enable
 *
 * action[en-context][name] = enable_artifact
 * action[en-context][order] = 1
 * action[en-context][param][] = context_ui
 *
 */
function enable_artifact_action($name)
{
    $args = func_get_args();
    $site_record = drush_get_context('FLOW_WIDGET_ALIAS', '@self');
    //@ToDo implement check if artifact is not present, then download
    drush_log('artifact ' . $name . ((drush_invoke_process($site_record, "pm-enable", $args) ? ' WAS ' : ' WAS NOT')) . ' enabled', 'notice');
}

function revert_feature_action($name)
{
    $args = func_get_args();
    $site_record = drush_get_context('FLOW_WIDGET_ALIAS', '@self');
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
    _log('variable "' . $name . '" was set to "' . $value . '"');
//    _log('variable - ' . $name . ((drush_invoke_process("@self", "vset", array($name,$value)) ? ' WAS ' : ' WAS NOT')) . ' set to '.$value);
}

/*
 * different configurations for tuning site to production. Here is the list
 * 1. Remove Update messages(disable "update" module) to stop security messages
 * 2. Turn off messages
 * 3. @ToDo turn on file caching and aggrigation
 * 4. @ToDo Disable devel related modules - devel_themer, devel, coffee,etc
 * 5. @ToDo create local writable dir and assign to admin/config/media/file-system for "Private File Sys Path", "Tempaorary Dir"
*/
function production_ready_action()
{
    //turn off update module
    _log('module - "update" ' . ((drush_invoke_process("@self", "dis", array('update')) ? ' WAS ' : ' WAS NOT')) . ' disabled');

    //turn off messages
    set_action('error_level', 0);
}

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

    _log('configuring jquery_update...');
    enable_artifact_action('jquery_update');
    set_action('jquery_update_compression_type', "min");
    set_action('jquery_update_jquery_cdn', "google");
    set_action('jquery_update_jquery_admin_version', "1.10");
    set_action('jquery_update_jquery_version', "1.10");
}