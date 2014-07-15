

function soc_menu_func()
{
	echo
	echo [21] 编译系统
	echo [22] push模块到手机sdcard
	echo [23] 运行模块
	echo [24] gitk
	echo
}


function build_system()
{
	cd $DBG_SYSTEM_DIR
	if [[ $TARGET_PRODUCT = "" ]];then
		source build/envsetup.sh
		breakfast su640
	fi
		brunch su640
	cd - 
}

function soc_handle_func()
{
		case $1 in
		21)	
			build_system;;
		22)	
			adb push $DBG_OUT_PATH /sdcard/out;;
		23)
			insmod /sdcard/out/lidbg_loader.ko;;
		24)
			cd $DBG_KERNEL_SRC_DIR && gitk& ;;
		*)
			echo
		esac
}


