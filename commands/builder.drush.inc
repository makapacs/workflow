<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');


function builder_drush_command()
{
    $items = array();
    $items['build-asset'] = array(
        'description' => "Builds asset from blueprint",
        'arguments' => array(
            'name' => 'Asset name you have given',
//            'alias' => 'alias for the site the artifacts needs to be deployed. Default is @self which is from current directory',
            'type' => 'Asset type such as widget, theme,etc.'
        ),
        'options' => array(
            'ver' => 'version of the new package ',
            'branch' => 'the branch of repo. this can also be package type such as basic, plus, ultimate,etc',
            'min' => 'specify whether include dependencies or only minimal no dependencies package',
            'make' => 'the make file used for building package. It overrides the make file provided by asset configurations'
        ),
        'examples' => array(
            'ba slider widget --ver=0.1.0' => 'builds deliverable for asset - slider by using blueprint located per conventions at $make_dir/type(s)/pacakge-name.make (see "Convetions" section for details)',
            'ba espresso theme --make=themes/designssquare-com-theme-espresso.make --ver=0.6' => 'builds deliverable for asset theme - espresso from blueprint provided at command line',
        ),
        'aliases' => array('ba'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    );
    return $items;
}

function drush_build_asset_init()
{
    load_workflow();
    load_slim_drupal();
//    load_drupal();
    read_config();
    init_inputs();
    read_artifacts();
    init_artifacts();
}

function drush_builder_build_asset($name = 'not-found', $type = 'module')
{
    drush_log('building "' . $name . '" asset with type - "' . $type . '"...', 'ok');


    //goes through different variations of distribution name and retrieves the appropriate one
    $package = get_distribution($name);
    $package_name = $package['name'];
    $dest = $package['path'] . '/' . $package_name;
    if (file_exists($dest)) {
        //clean
        remove_distribution($package);
    }

//loop through types of artifacts and builds one by one
    foreach (_all_artifact_types() as $key => $artifact_type) {
        drush_log('looping over artifact type: ' . $artifact_type, 'notice');

        $min = drush_get_context('WF_ASSET_MIN', FALSE);
        if (!$min) {
            $all_artifacts = get_artifacts_from_blueprint($artifact_type);
            //read all artifacts from make file and loop through building one at the time
            if (isset($all_artifacts) && is_array($all_artifacts) && !empty($all_artifacts)) {
                foreach ($all_artifacts as $key => $artifact) {
                    drush_log('baking...' . $artifact['name'], 'ok');
                    build_artifact($artifact, $dest);
                }
            } else {
                drush_log('INFO: no artifacts of type ' . $artifact_type . ' is specified', 'ok');
            }
        } else {
            //retrieve the  main artifact
            $artifact = get_artifact_by_name($name);
            //build the main artifact and don't include dependencies
            build_artifact($artifact, $dest);
        }
    }

//copy documentation
    drush_log(dt('copying documentation...'), 'ok');
    $artifact = get_artifact_by_name($name);
    $src = $dest . '/' . $type . 's/' . $artifact['final_name'];
    _copy_documentation($src, $dest);

    //compile and copy make file
    drush_log(dt('compiling and copying make files...'), 'ok');
    $make_file_src = get_make_file($name);
    $final_make = $package['path'].'/' . $package_name . '/'.$package_name.'.make';
    execute_command('sudo cp '.$make_file_src .' ' . $final_make);
    execute_command('sudo chmod -R 777 '.$final_make);

    //append actions from default make file
    append_content(drush_get_context('WF_DEFAULT_MAKE'), $final_make, 'WF_START', 'WF_END');

    //remove git files "sudo rm -rf .git"
    drush_log(dt('removing git repo info...'), 'ok');
    execute_command('sudo rm -rf ' . $dest . '/*/*/.git');
    execute_command('sudo rm -rf ' . $dest . '/*/*/.gitignore');

//create compressed file
    drush_log(dt('taring package...'), 'ok');
    drush_op('chdir', $package['path']);
    execute_command('sudo tar -czf ' . $package_name . '.tar.gz ' . $package_name);
//execute_command('tar -czf ' . $package_name . '.tar.gz -C '.$package['path'].' '.$dest);

//create zip file
    drush_log(dt('ziping package...'), 'ok');
    drush_op('chdir', $package['path']);
//to avoid full path, we go to distribution. this can be improved
    execute_command('sudo zip -r ' . $package_name . '.zip ' . $package_name);

    drush_log(dt('Done building @package', array('@package' => $package_name)), 'ok');
}