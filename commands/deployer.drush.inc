<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');


function deployer_drush_command()
{
    $items = array();
    $items['deploy-artifact'] = array(
        'description' => "downloads and installs artifacts with its dependencies. Depending of environment(i.e. test, dev, stage), the process differs",
        'arguments' => array(
            'name' => 'artifact name',
            'alias' => 'alias for the site the artifacts needs to be deployed. Default is @self which is from current directory',
        ),
        'options' => array(
            '--ver' => 'version of the new package',
            '--branch' => 'the branch of repo. this can also be package type such as basic, plus, ultimate,etc',
            '--min' => 'specify whether include dependencies or only minimal no dependency artifacts',
            '--env' => 'the environment such as test, dev, stage to deploy artifact.',
        ),
        'examples' => array(
            'drush da espresso @workflow' => 'downloads and installs the artifact theme - espresso for site with alias @workflow and branch - master in STAGE format',
            'drush da espresso @workflow --branch=plus --env=dev' => 'downloads and installs the artifact theme - espresso and branch - plus for site with alias @workflow in DEV format',
            'drush da espresso @workflow --branch=plus --ver=0.0 --env=test' => 'downloads and installs the artifact theme - espresso and branch - plus for site with alias @workflow in TEST format',
        ),
        'aliases' => array('da'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    );
    return $items;
}

function drush_deploy_artifact_init()
{
    load_workflow();
    load_slim_drupal();
    read_config();
    read_artifacts();
    init_inputs();
    init_artifacts();
//    init_workflow();
}

function drush_deployer_deploy_artifact($name, $alias_name)
{
    drush_log('deploying "' . $name . '" artifact...', 'ok');

// check if we can bootstrap
    $alias = drush_get_context('FLOW_WIDGET_ALIAS');
    if (empty($alias)) {
        return drush_set_error('WORKFLOW_DEPLOY_ERROR', dt('ERROR: I can\'t bootstrap from the location provided' ));
    }

    $root_dir = $alias['root'];
        $distribution = get_distribution($name);
//let's jump to our site directory before we do anything else
    drush_op('chdir', $root_dir);

    $env = drush_get_context('FLOW_WIDGET_ENV', 'dev');
    drush_log('***'.strtoupper($env).' deployment***','ok');

//loop through different type of artifacts and perform delete, build
        foreach (_all_artifact_types() as $key => $artifact_type) {
            drush_log('looping over artifact type: ' . $artifact_type, 'notice');

        switch ($env) {
            case 'dev':
                drush_log('removing artifacts previously installed with type:' . $artifact_type .'... ', 'ok');
                //removing artifacts based on whats in the package
                remove_make_artifacts($artifact_type);
                //link artifacts from working directory based on make file
                drush_log('done removing, start to build all artifacts with type: ' . $artifact_type .'...', 'ok');
                foreach (get_artifacts_from_blueprint($artifact_type) as $key => $item) {
                    $artifact = get_artifact_by_name($item['name']);
                    //destination per Drupal file structure
                    $drup_type = ($artifact_type != 'theme') ? 'module' : $artifact_type;
                    $dest = $root_dir . '/sites/all/' . $drup_type . 's/' . $artifact['final_name'];
                    //check to see if located at location per convention. if not then set work_dir + '/' + work_dir_name
                    $source = (file_exists($artifact['work_dir'].'/'.$artifact_type.'s')) ? $artifact['work_dir'].'/'.$artifact_type.'s/'.$artifact['work_dir_name'] : $artifact['work_dir'] . '/' . $artifact['work_dir_name'];
                    drush_log('Linking ' . $artifact_type . ' - ' . $artifact['final_name'] . ' from ' . $source . ' to ' . $dest . '...', 'notice');

                    if (file_exists($source)) {
                        execute_command('sudo ln -s ' . $source . ' ' . $dest);
                    } else {
                        drush_log(dt('ERROR:source - ' . $source . ' does not exist'), 'ok');
                    }
                }
                break;
            case 'test':
                //test env
                $dist_full_path = $distribution['path'] . '/' . $distribution['name'];
                $type_test = ($artifact_type != 'theme') ? 'module' : $artifact_type;
                $source_dir = $dist_full_path . '/' . $type_test . 's';
                drush_log('locating artifacts at '.$source_dir, 'ok');
                if (file_exists($source_dir)) {
                    //removing artifacts
                    drush_log('start removing previously installed artifacts with type - '.$artifact_type.'... ', 'ok');
                    remove_package_artifacts($dist_full_path, $artifact_type);

                    //adding by copying from distribution package
                    drush_log('done removing, start building artifacts with type - ' . $artifact_type . '...', 'ok');
                    execute_command('sudo cp -R ' . $source_dir . '/ ' . $root_dir . '/sites/all/' . $type_test . 's/');
                }else{
                    drush_log(dt('INFO: There are no artifacts of type: '.$artifact_type.' to build'),'ok');
                }
                break;
            default:
//            case 'stage':
                drush_log(dt('start removing previously installed artifacts of type - ' . $artifact_type . '...'),'ok');
                //removing artifacts based on what declared in make file
                remove_make_artifacts($artifact_type);

                //adding new modules from remote github
                drush_log('done removing, start building artifacts of type - ' . $artifact_type . '...', 'ok');
                foreach (get_artifacts_from_blueprint($artifact_type) as $key => $artifact) {
                    build_artifact($artifact, 'sites/all');
                }

                break;
        }
    }

//download all projects specified in the .make file
    $make_file = get_make_file($name);
    $build_file_parsed = drupal_parse_info_file($make_file);
    $project_modules = _project_modules_from_make($build_file_parsed);

    foreach ($project_modules as $key => $module) {
        //download
        drush_log('module - ' . $module . ((drush_invoke_process("@self", "pm-download", array($module)) ? ' WAS ' : ' WAS NOT')) . ' downloaded', 'notice');
    }

    drush_log(dt('Done deploying @name', array('@name' => $name)), 'ok');
}