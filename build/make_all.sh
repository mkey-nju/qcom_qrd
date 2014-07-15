
source ./env_entry.sh

function build_dir()
{
	echo -e "\033[44;37mmake $1 \033[0m"
	cd  $1
	make modules -j8

	if [ $? = 0 ]; then
		echo
	else
		echo -e "\033[41;37mmake $1 error!!\033[0m"
		make modules | grep -C 1 "error:"
		echo -e "\033[41;37mmake $1 error!!\033[0m"
		read get_key
	fi
}

clear
build_dir $DBG_SOC_PATH/$DBG_SOC
build_dir $DBG_CORE_PATH
build_dir $DBG_DRIVERS_PATH
