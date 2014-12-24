<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');


function deployer_drush_command()
{
    $items = array();
    $items['deploy-asset'] = array(
        'description' => "downloads and installs asset and all of its dependent artifacts. Depending of environment(i.e. test, dev, stage), the process differs",
        'arguments' => array(
            'alias' => 'alias for the site the asset needs to be deployed. Default is @self which is from current directory',
        ),
        'options' => array(
            'ver' => 'version of the new package',
            'branch' => 'the branch of repo. this can also be package type such as basic, plus, ultimate,etc',
            'min' => 'specify whether include dependencies or only minimal no dependent artifacts',
            'env' => 'the environment such as test, dev, stage to deploy asset.',

            /*different ways to provide blueprint - make file*/
            'package' => 'full or relative path to compressed package or uncompressed package directory that you wan to deploy from. This overrides the package specified by asset.',
            'make' => 'full or relative path to make file that you wan to deploy from. This overrides the make file specified by asset.',
            'asset' => 'name of the asset to deploy'
        ),
        'examples' => array(
            'drush da @workflow --asset=espresso' => 'downloads and installs the asset theme - espresso for site with alias @workflow and branch - master in STAGE format',
            'drush da @workflow --asset=espresso --ver=0.5 --env=dev' => 'downloads and installs the asset theme - espresso and branch - basic for site with alias @workflow in DEV format',
            'drush da @workflow --asset=espresso --branch=plus --ver=0.0 --env=test ' => 'downloads and installs the asset theme - espresso and branch - plus for site with alias @workflow in TEST format',
            'drush da @workflow --package=path/to/package.zip' => 'deploys from the package as specified by flag --package in TEST env',
            'drush da @workflow --make=path/to/some.make' => 'deploys from github in STAGE env',
        ),
        'aliases' => array('da'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    );
    return $items;
}

function drush_deploy_asset_init()
{
    load_workflow();
    load_slim_drupal();
    read_config();
    init_inputs();
    read_artifacts();
    init_artifacts();
//    init_workflow();
}

function drush_deployer_deploy_asset($alias_name)
{
    inform_make_supplied();

// check if we can bootstrap
    $alias = drush_get_context('WF_ALIAS');
    if (empty($alias)) {
        return drush_set_error('WORKFLOW_DEPLOY_ERROR', dt('ERROR: I can\'t bootstrap from the location provided'));
    }

    $root_dir = $alias['root'];


//let's jump to our site directory before we do anything else
    drush_op('chdir', $root_dir);

    $env = wf_var('WF_ENV', 'dev');
    drush_log('***' . strtoupper($env) . ' deployment***', 'ok');

//loop through different type of artifacts and perform delete, build
    foreach (_all_artifact_types() as $key => $artifact_type) {
        drush_log('looping over artifact type: ' . $artifact_type, 'notice');

        switch ($env) {
            case 'dev':
                drush_log('removing artifacts previously installed with type:' . $artifact_type . '... ', 'ok');
                //removing artifacts based on whats in the package
                remove_make_artifacts($artifact_type);
                //link artifacts from working directory based on make file
                drush_log('done removing, start to build all artifacts with type: ' . $artifact_type . '...', 'ok');
                foreach (get_artifacts_from_blueprint($artifact_type) as $key => $item) {
                    $artifact = get_artifact_by_name($item['name']);
                    //destination per Drupal file structure
                    $drup_type = ($artifact_type != 'theme') ? 'module' : $artifact_type;
                    $dest = $root_dir . '/sites/all/' . $drup_type . 's/' . $artifact['final_name'];
                    //check to see if located at location per convention. if not then set work_dir + '/' + work_dir_name

//                    $source = (file_exists($artifact['work_dir'] . '/' . $artifact_type . 's')) ? $artifact['work_dir'] . '/' . $artifact_type . 's/' . $artifact['work_dir_name'] : $artifact['work_dir'] . '/' . $artifact['work_dir_name'];
                    $source = get_work_dir($artifact);
                    drush_log('Linking ' . $artifact_type . ' - ' . $artifact['final_name'] . ' from ' . $source . ' to ' . $dest . '...', 'notice');

                    if (file_exists($source)) {
                        execute_command('sudo ln -s ' . $source . ' ' . $dest);
                    } else {
                        drush_log(dt('ERROR:source - ' . $source . ' does not exist'), 'ok');
                    }
                }
                break;
            case 'test':
                //check if flag - package is set then don't use artifact look up package
                if($package_dir = drush_get_context('WF_ASSET_PACKAGE',FALSE)){
                    $dist_full_path = $package_dir;
                }elseif($name = wf_var('WF_ASSET_NAME','no-asset')){
                    $distribution = get_distribution($name);
                    $dist_full_path = $distribution['path'] . '/' . $distribution['name'];
                }
                //if no valid source for package specified, then quit
                if(!file_exists($dist_full_path)){
                    return drush_set_error('WORKFLOW_DEPLOYER_ERROR', dt('ERROR: no valid package specified...@package', array(
                        '@package' => $dist_full_path,
                    )));
                }

                drush_log(dt('deploying from @dist...', array('@dist' => $dist_full_path)),'notice');

                //test env
                $type_test = ($artifact_type != 'theme') ? 'module' : $artifact_type;
                $source_dir = $dist_full_path . '/' . $type_test . 's';
                drush_log('locating artifacts from ' . $source_dir, 'ok');
                if (file_exists($source_dir)) {
                    //removing artifacts
                    drush_log('start removing previously installed artifacts with type - ' . $artifact_type . '... ', 'ok');
                    remove_package_artifacts($dist_full_path, $artifact_type);

                    if(!file_exists($source_dir)){
                        drush_die('ERROR: artifacts to be deployed are not present','ok');
                    }

                    //adding by copying from distribution package
                    drush_log('done removing, start building artifacts with type - ' . $artifact_type . '...', 'ok');
                    execute_command('sudo cp -R ' . $source_dir . '/* ' . $root_dir . '/sites/all/' . $type_test . 's/');
                } else {
                    drush_log(dt('INFO: There are no artifacts of type: ' . $artifact_type . ' to build'), 'ok');
                }
                break;
            default:
//            case 'stage':
                drush_log(dt('start removing previously installed artifacts of type - ' . $artifact_type . '...'), 'ok');
                //removing artifacts based on what declared in make file
                remove_make_artifacts($artifact_type);

                //adding new modules from remote github
                drush_log('done removing, start building artifacts of type - ' . $artifact_type . '...', 'ok');
                foreach (get_artifacts_from_blueprint($artifact_type) as $key => $artifact) {
                    build_artifact($artifact, 'sites/all/'.get_artifact_final_dir($artifact['name']));
                }
                break;
        }
    }

    drush_log(dt('Done deploying @name', array('@name' => $name)), 'ok');
}