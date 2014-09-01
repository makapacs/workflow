;;@alias - alias
;;@package - dir_to_package
;;@tags - config,init,revert plus user specified
;;@env - environment

action[install][name]=install-site
action[install][order]=0
action[install][tag]=install
action[install][param][]=@alias

action[deploy][name]=deploy-artifact
action[deploy][order]=1
action[deploy][tag]=deploy
action[deploy][param][]=@package
action[deploy][param][]=@alias
action[deploy][flag][env]=@env
action[deploy][flag][ver]=@ver

action[configure][name]=configure-artifact
action[configure][order]=2
action[configure][param][]=@artifact
action[configure][param][]=@alias
action[configure][flag][tag]=config,init,revert
