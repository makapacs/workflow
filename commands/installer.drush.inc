<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');


function installer_drush_command()
{
    $items = array();
    $items['install-site'] = array(
        'description' => "downloads and installs the drupal instance at the location of alias per blue print specified",
        'arguments' => array(
            '@alias' => 'alias name of the instance to install. Make sure to include "@" in front',
//            'makefile' => 'blueprint of drupal installation'
        ),
        'options' => array(
            'env' => 'environment of the site such as dev, stage, test, etc. By default it\'s stage',
            'su' => 'site admin user. By default, it\'s "admin"',
            'su-pw' => 'site admin user password. By default, it\'s "password"',
            'db-user' => 'database user name. If not set, will use the general db user configured part of WorkFlow external configurations',
            'db-pwd' => 'database password. If not set, will use the general db password configured part of WorkFlow external configurations',
            'db-name' => 'the name of database',
            'db-address' => 'the address of database',
            'dest' => 'full path to destination directory. If not specified, it will use the "root" value from the site alias configurations',

            /*different ways to provide blueprint - make file*/
            'package' => 'full or relative path to compressed package or uncompressed package directory containing make file you like to be used in this installation. This overrides the package specified by asset.',
            'make' => 'full or relative path to make file that you want to be used for installation. This overrides the make file specified by asset.',
            'asset' => 'name of the asset containing make file used for installing new site'

        ),
        'examples' => array(
            'drush is @awesomeness' => 'downloads and installs the awesomeness based on the make file declared in the alias or in the external configurations',
            'drush is --make-file=path/to/make-file.make' => 'downloads and installs the awesomeness from make file provided'
        ),
        'aliases' => array('is'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    );
    return $items;
}

function drush_install_site_init()
{
//    load_workflow();
//    read_config();
//    init_workflow();

    read_config();
//    read_artifacts();
    init_inputs();
    load_slim_drupal();
//    load_drupal();
    load_workflow();
}

function drush_installer_install_site($alias='@self')
{
    drush_log('installing drupal instance...', 'ok');

    //retrieve db_user & db_password
//    $site_record = drush_sitealias_get_record($alias);
    $site_record = wf_var('WF_ALIAS');
    //retrieve alias

    $env = get_env();
    $db_name = get_db_credential('db-name');
    $db_user = get_db_credential('db-user');
    $db_pwd = get_db_credential('db-pwd');
    $address = ($address = get_db_credential('db-address')) ? $address : 'localhost';

    $is_db_url = FALSE;
    if($db_url = drush_get_option('db-url', TRUE)){
        if(isset($site_record['db-url']) && !empty($site_record['db-url'])){
            //db_pwd not provided ...see if specified in alias configs.
            $db_url = $site_record['db-url'];
            //db-url is provided
            $is_db_url = TRUE;
        }else{
            //$db_url not provided in alias configs...construct from db-user, db-pwd and db-name
            $db_url = 'mysql://' . $db_user . ':' . $db_pwd . '@'.$address.'/' . $db_name;
        }
    }else{
        //db-url is provided
        $is_db_url = TRUE;
    }

    //if db is specified by url, extract the db credentials
    if($is_db_url){
        $db_conf = drush_convert_db_from_db_url($db_url);
        $db_user = $db_conf['user'];
        $db_pwd = $db_conf['pass'];
        $db_name = $db_conf['database'];
        $address = $db_conf['host'];
    }

    drush_log(dt('DB Credentials: user=@user, pass=@pwd, db-name=@name, db-address=@address', array('@user' => $db_user, '@pwd' => $db_pwd, '@name' => $db_name, '@address' => $address)),'notice');


    # Site Configurations
##########################################################

    if($site_user = drush_get_option('su', TRUE)){
        //site_user not provided...see if specified in alias configs.
        if(isset($site_record['su']) && !empty($site_record['su'])){
            $site_user = $site_record['su'];
        }else{
            //not provided in alias...default to admin
             $site_user = 'admin';
        }
    }

    if($site_pwd = drush_get_option('su-pw', TRUE)){
        if(isset($site_record['su-pw']) && !empty($site_record['su-pw'])){
            //$site_pwd not provided ...see if specified in alias configs.
            $site_pwd = $site_record['su-pw'];
        }else{
            //$site_pwd not provided in alias configs...default to 'password'
            $site_pwd = 'password';
        }
    }

    drush_log(dt('SITE Credentials: user=@user, pass=@pwd ', array('@user' => $site_user, '@pwd' => $site_pwd)),'notice');

    $AdminUsername = $site_user;
    $AdminPassword = $site_pwd;
    $adminEmail = "admin@example.com";
    $siteName = $alias . " Site";
    $siteSlogan = $env . " - " . $alias . "";
    $siteLocale = "en";
##########################################################

#Download
#########################################################
    //destination
    $default_dest = get_destination();


    if(file_exists($default_dest)){
        drush_log(dt('destination - @dest exists. Removing for clean install', array('@dest' => $default_dest)),'ok');
        execute_command('sudo rm -R ' . $default_dest);
    }

    drush_log(dt('creating destination - @dest ...', array('@dest' => $default_dest)),'ok');
    execute_command('sudo mkdir ' . $default_dest);
    execute_command('sudo chmod -R 777 ' . $default_dest);

    //makefile
    $default_make = get_make_file();

# Install core
##########################################################
// let's jump to our site directory before we do anything else, because it may be other than alias specified
//    drush_op('chdir', $default_dest)
    chdir($default_dest);
    //download Drupal and modules per make file
    drush_invoke_process('@self', 'make', array($default_make), array('yes' => 1, 'no-core'=>0, 'debug'=>0));

//install
    $status = drush_invoke_process('@self', 'site-install', array('standard'), array(
            'account-mail'=> $adminEmail,
            'account-name' => $AdminUsername,
            'account-pass' => $AdminPassword,
            'site-name' => $siteName,
            'locale' => $siteLocale,
            'db-url' => $db_url,
        )
    );
    drush_log('Site Install - ' . (($status) ? 'SUCCESS' : 'FAILURE'), 'ok');

    drush_log(dt('Done installing drupal instance at @location', array('@location' => $default_dest)), 'ok');
}