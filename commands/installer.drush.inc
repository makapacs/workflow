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
            'db-name' => 'the name of database. By default, it is constructed $env + "_" + $alias "_" + db or how its specified in alias configration - "db_name"',
            'make' => 'make file for the drupal installation. It can also be configured either at the alias site and external env configurations',
            'dest' => 'full path to destination directory. If not specified, it will use the "root" value from the site alias configurations'
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
    $site_record = drush_get_context('WF_ALIAS');
    //retrieve alias
    if($env = drush_get_option('env', TRUE)){
        //$env not provided...see if specified in alias config.
        if(isset($site_record['env']) && !empty($site_record['env'])){
            $env = $site_record['env'];
        }else{
            //not provided in alias...default to 'stage'
            $env = 'stage';
        }
    }

    if($db_name = drush_get_option('db-name', TRUE)){
        //$db_name not provided...see if specified in alias configs.
        if(isset($site_record['db-name']) && !empty($site_record['db-name'])){
            $db_name = $site_record['db-name'];
        }else{
            //not provided in alias...construct from env, alias values
            $db_name = $env . '_' . $site_record['#name'] . '_db';
        }
    }


    if($db_user = drush_get_option('db-user', TRUE)){
        //db_user not provided...see if specified in alias configs.
        if(isset($site_record['db-user']) && !empty($site_record['db-user'])){
            $db_user = $site_record['db-user'];
        }else{
            //not provided in alias...see if in general config. otherwise construct from env,alias
            ($db_user = drush_get_context('WF_DB_USER', FALSE)) ? /* using from external configs*/ : $db_user = $env . '_' . $alias;
        }
    }

    if($db_pwd = drush_get_option('db-pwd', TRUE)){
        if(isset($site_record['db-pwd']) && !empty($site_record['db-pwd'])){
            //db_pwd not provided ...see if specified in alias configs.
            $db_pwd = $site_record['db-pwd'];
        }else{
            //db_pwd not provided in alias configs...see if specified in general configs. if not, use 'password'
            ($db_pwd = drush_get_context('FW_DB_PWD', FALSE)) ? /* using from external configs*/ : $db_pwd = 'password';
        }

    }

    $is_db_url = FALSE;
    if($db_url = drush_get_option('db-url', TRUE)){
        if(isset($site_record['db-url']) && !empty($site_record['db-url'])){
            //db_pwd not provided ...see if specified in alias configs.
            $db_url = $site_record['db-url'];
            //db-url is provided
            $is_db_url = TRUE;
        }else{
            //$db_url not provided in alias configs...construct from db-user, db-pwd and db-name
            $db_url = 'mysql://' . $db_user . ':' . $db_pwd . '@127.0.0.1/' . $db_name;
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
    }

    drush_log(dt('DB Credentials: user=@user, pass=@pwd, db-name=@name', array('@user' => $db_user, '@pwd' => $db_pwd, '@name' => $db_name)),'notice');

    //check if database exists or can connect with current credentials
//    $con = mysqli_connect("localhost", $db_user, $db_pwd, $db_name);
//    if (mysqli_connect_errno()) {
//        drush_log(dt("Failed to connect to MySQL: @connect", array('@connect' => mysqli_connect_error())), 'notice');
//        drush_log(dt('database @db does not exist ....creating', array('db' => $db_name)), 'ok');
        //create database. @NO Need because site-install will create
//        $ret = drush_invoke_process('@self', 'sql-create', array(), array(
//            'db-su' => $db_user,
//            'db-su-pw' => $db_pwd,
//            'db-url' => $db_url,
//        ));
//        drush_log(dt('Sql-create: @ret', array('@ret' => implode(',',$ret))), 'debug');
//    } else {
//        drush_log(dt('database @db exist....skip creating', array('db' => $db_name)), 'ok');
//    }


//    $debug_mode = TRUE;


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
    if($default_dest = drush_get_option('dest', TRUE)){
        if(isset($site_record['root']) && !empty($site_record['root'])){
            //$default_dest not provided ...see if specified in alias configs.
            $default_dest = $site_record['root'];
        }else{
            //$default_dest not provided in alias configs...defaulting to root/deploys/env-alias
            $default_dest = drush_get_context('WF_WORK_DIR', FALSE) . '/deploys/'.$env . '-' . $alias;
//            return drush_set_error('WORKFLOW_INSTALLER_ERROR', dt('ERROR: the destination directory is not provided' ));
        }
    }

    if(file_exists($default_dest)){
        drush_log(dt('destination - @dest exists. Removing for clean install', array('@dest' => $default_dest)),'ok');
        execute_command('sudo rm -R ' . $default_dest);
    }

    drush_log(dt('creating destination - @dest ...', array('@dest' => $default_dest)),'ok');
    execute_command('sudo mkdir ' . $default_dest);
    execute_command('sudo chmod -R 777 ' . $default_dest);

    //makefile
    if($default_make = drush_get_option('make-file', TRUE)){
        if(isset($site_record['make-file']) && !empty($site_record['make-file'])){
            //$default_make not provided ...see if specified in alias configs.
            $default_make = $site_record['make-file'];
        }else{
            //$default_make not provided in alias configs...'
            return drush_set_error('WORKFLOW_INSTALLER_ERROR', dt('ERROR: the make file not provided' ));
        }

    }

//    $dest = ($build_dest = drush_get_option('build-dest', FALSE)) ? $build_dest : $default_dest;
//    drush_log(dt('Dest folder: @dest', array('@dest' => $dest )), 'debug');
//    $status = _process_make_file($default_make, $default_dest, array('no-core' => 0));
//    $proc_status = ($status) ? "SUCCESSFUL" : "FAILED";
//    drush_log(dt('Make Download @st',array('@st' => $proc_status)), 'notice');

# Database
##########################################################
//    $dbHost="localhost";
//    $dbName= ($dbname = drush_get_option('db-name', FALSE)) ? $dbname : $db_name;
//    $dbUser= ($dbuser = drush_get_option('db-user', FALSE)) ? $dbuser : $db_user;
//    $dbPassword = ($dbpass = drush_get_option('db-pass', FALSE)) ? $dbpass : $db_password;
//    $dbPort ="3306";
//    $db_url = ($dburl = drush_get_option('db-url', FALSE)) ? $dburl : 'mysql://'.$dbUser.':'.$dbPassword.'@'.$dbHost.':' . $dbPort . '/' . $dbName;
##########################################################


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