<?php
require_once(dirname(__FILE__) . '/../lib/workflow-lib.inc');


function constructor_drush_command()
{
    $items = array();
    $items['construct-container'] = array(
        'description' => "sets up environment for installing drupal instance. It includes creating Database,configuring virtual host,etc",
        'arguments' => array(
            '@alias' => 'drush alias with extended info(i.e. db-name,db-user,etc) from workflow. Make sure to include "@" in front',
        ),
        'options' => array(
            'db-user' => 'database user name. If not set, will use the general db user configured part of WorkFlow external configurations',
            'db-pwd' => 'database password. If not set, will use the general db password configured part of WorkFlow external configurations',
            'db-name' => 'the name of database',
            'db-address' => 'the address of database',
            'dest' => 'full path to destination directory. If not specified, it will use the "root" value from the site alias configurations',
        ),
        'examples' => array(
            'drush cco @workflow' => 'creates database with name @workflow[db-name], configures virtual host for location @workflow[root],etc',
            'drush cco @workflow --db-name=test' => 'creates database with user provided name - db-name, configures virtual host for location @workflow[root],etc',
        ),
        'aliases' => array('cco'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    );
    return $items;
}

function drush_construct_container_init()
{
//    load_workflow();
//    read_config();
//    init_workflow();

    read_config();
//    read_artifacts();
    init_inputs();
//    load_slim_drupal();
//    load_drupal();
    load_workflow();
}

function drush_constructor_construct_container($alias = '@self')
{
    $site_record = wf_var('WF_ALIAS');

    if (!isset($site_record) || empty($site_record)) {
        drush_die('Error: the alias is not specified', 0);
    }
    $db_admin = wf_var('WF_DB_SUPER_USER');
    if (!isset($db_admin) || empty($db_admin)) {
        drush_die('Error: the DB SUPER User is not specified. To Set up, use variable "db-super-user" in the WorkFlow external configurations', 0);
    }
    $db_admin_pwd = wf_var('WF_DB_SUPER_PWD');
    if (!isset($db_admin_pwd) || empty($db_admin_pwd)) {
        drush_die('Error: the DB SUPER User password not specified. To Set up, use "db-super-pwd" in WorkFlow external configurations', 0);
    }

    drush_log('constructing container for alias...', 'ok');
    $db_name = get_db_credential('db-name');
    $db_user = get_db_credential('db-user');
    $db_pwd = get_db_credential('db-pwd');
    $host = ($host = get_db_credential('db-address')) ? $host : 'localhost';

    //if db is specified by url, extract the db credentials
    if ($db_url = get_db_url()) {
        $db_conf = drush_convert_db_from_db_url($db_url);
        $db_user = $db_conf['user'];
        $db_pwd = $db_conf['pass'];
        $db_name = $db_conf['database'];
        $host = $db_conf['host'];
    }

    drush_log(dt('DB Credentials: user=@user, pass=@pwd, db-name=@name, db-address=@address', array('@user' => $db_user, '@pwd' => $db_pwd, '@name' => $db_name, '@address' => $host)), 'notice');

    $db_output = setup_db(array(
        'host' => $host,
        'root_usr' => $db_admin,
        'root_pwd' => $db_admin_pwd,
        'db_name' => $db_name,
        'db_usr' => $db_user,
        'db_pwd' => $db_pwd,
    ));

    if ($db_output) {
        drush_log('db - ' . $db_name . ' was created successfully', 'ok');
    } else {
        drush_log('Failed to create db - ' . $db_name, 'ok');
    }

    //create destination dir
    $default_dest = get_destination();
    drush_log('creating dir - '.$default_dest.'...','ok');
    execute_command('sudo mkdir -p '.$default_dest);

    //prepare permissions for install
    drush_log('set write permissions to dir - '.$default_dest.'...','ok');
    execute_command('sudo chomod -R 777 '.$default_dest);
//    chdir($default_dest);

    //@ToDo implment vhost setup here



    drush_log(dt('Done constructing container for alias @alias', array('@alias' => $alias)), 'ok');
    drush_log(dt('try spinning new drupal "drush spin-it @some"', array('@some' => $alias)), 'ok');
}
