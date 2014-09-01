<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');
require_once(dirname(__FILE__) . '/../actions/actions-core.inc');


function profiler_drush_command()
{
    $items = array();
    $items['profile-package'] = array(
        'description' => "install, deploy and configure a drupal instance from a package",
        'arguments' => array(
            'profile' => 'name of profile. It is the name of the profile make file in the /profile dir',
            'alias' => 'alias for the site the action is applied to. Default is @self which is from current directory. ',
        ),
        'options' => array(
            '--tag' => 'list of one or more tags to associated with actions to execute on the package in addition to "config,init,revert" at the configuration stage',
            '--env' => 'the environment such as test, dev, stage used at deployment stage',
            '--package' => 'path to the package dir used for this profile'
        ),
        'examples' => array(
            'pp installer-basic @workflow --env=test --pacakge=path/to/package/dir' => 'installs new site, deploys artifacts of the package and configures per actions with tags config,init,revert',
        ),
        'aliases' => array('pp'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
//    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
    );
    return $items;
}

function drush_profile_package_init()
{

    read_config();
    read_artifacts();
    init_inputs();
    load_drupal();
    load_workflow();
    init_artifacts();
}

function drush_profiler_profile_package($name = 'not-found', $alias = '@self')
{
    drush_log(dt('executing profile  - @profile ...', array('@profile' => $name)), 'ok');

// check if we can bootstrap
    $alias_record = drush_get_context('FLOW_WIDGET_ALIAS');
    $active_tags = drush_get_context('FLOW_WIDGET_TAG');

    if (empty($alias_record)) {
        drush_die("I can't bootstrap from the current location.", 0);
    }

//    drush_bootstrap_max_to_sitealias($alias_record);

//let's jump to our site directory before we do anything else
//    drush_op('chdir', $alias_record['root']);

    $profile_blueprint = dirname(__FILE__) . '/../profiles/'.drush_get_context('FLOW_PROFILE_NAME').'.make';

    if(!file_exists($profile_blueprint)){
        return drush_set_error('WORKFLOW_PROFILE_ERROR', dt('ERROR: profile blueprint - '.$profile_blueprint.' doesn\'t exist' ));
    }

    $all_actions = get_actions_from_make($profile_blueprint);
    //grab actions from package make file
    $all_actions = $all_actions + get_actions_from_make(drush_get_context('FLOW_PACKAGE_DIR'));

    foreach($all_actions as $action){
        print_r($action);
    }

    drush_log(dt('done executing profile  - @profile', array('@profile' => $name)), 'ok');
}

