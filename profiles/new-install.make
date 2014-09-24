;;@alias - alias
;;@package - dir_to_package
;;@tags - config,init,revert plus user specified
;;@env - environment
;;@make - the path to make file
;;@asset -  name of the asset
;;@ver - the version of the asset
;;@branch -  the branch or type
;;@db-user - database user
;;@db-pwd - database password
;;@db-name - database name
;;@db-address - database address

action[install][name]=install-site
action[install][order]=0
action[install][tag]=install
action[install][param][]=@alias
action[install][flag][make]=@make
action[install][flag][package]=@package
action[install][flag][db-user]=@db-user
action[install][flag][db-pwd]=@db-pwd
action[install][flag][db-name]=@db-name
action[install][flag][db-address]=@db-address

action[deploy][name]=deploy-asset
action[deploy][order]=1
action[deploy][tag]=deploy
action[deploy][param][]=@alias
action[deploy][flag][package]=@package
action[deploy][flag][env]=@env
action[deploy][flag][make]=@make
action[deploy][flag][ver]=@ver
action[deploy][flag][asset]=@asset
action[deploy][flag][branch]=@branch

action[configure][name]=configure-asset
action[configure][order]=2
action[configure][param][]=@alias
action[configure][flag][tag]=@tags
action[configure][flag][package]=@package
action[configure][flag][make]=@make
action[configure][flag][asset]=@asset
action[configure][flag][ver]=@ver
action[configure][flag][branch]=@branch