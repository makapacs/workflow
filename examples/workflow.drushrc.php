<?php
/**EXTERNAL WORKFLOW ENVIRONMENT CONFIGURATION**/
$workflow_config = array();
$workflow_config['work_dir'] = '/Users/owner/ds_workflow/work-dir';
$workflow_config['make_root'] = '/Users/owner/ds_workflow/blueprints';
$workflow_config['dist'] = '/Users/owner/ds_workflow/dist';
$workflow_config['package_prefix'] = 'designssquare-com-';
$workflow_config['make_file'] = '/Users/owner/ds_workflow/blueprints/build-drupal-core.make';

/* Database user used for alias if no db-user specified
 */
$workflow_config['db-user'] = 'workflow_user';

/* Database user used for alias if no db-pwd specified
 */
$workflow_config['db-pwd'] = 'password1';

/* Database root user and password with privileges to create databases and users.
 */
$workflow_config['db-super-user'] = 'root';
$workflow_config['db-super-pwd'] = 'password2';


$artifact_types = array();
$artifact_types[] = 'widget';
$artifact_types[] = 'theme';
