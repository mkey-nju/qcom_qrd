
function print_dir_num()
{
	echo
	echo "<1> "$DBG_ROOT_PATH
	echo "<2> "$DBG_SYSTEM_DIR
	echo "<3> "$RELEASE_REPOSITORY
}

function get_dir()
{
	read -p "输入目录标号：" dir_num
	case $dir_num in
	1)	
		echo $DBG_ROOT_PATH;;
	2)
		echo $DBG_SYSTEM_DIR;;
	3)
		echo $RELEASE_REPOSITORY;;
	*)
		echo
	esac 
}

function common_menu()
{
	echo [81] pull                       
	echo [82] push
	echo [83] gitk
	echo [84] git log
	echo [85] git reset
	echo [86] nautilus
}

function common_handle()
{
	cd $RELEASE_REPOSITORY
	print_dir_num
	case $1 in
	81)
		dir=$(get_dir)
		case $dir in
			$DBG_ROOT_PATH)	
				cd $dir && lidbg_pull;;
			$DBG_SYSTEM_DIR)
				cd $dir && expect $DBG_TOOLS_PATH/pull;;
			$RELEASE_REPOSITORY)
				cd $dir && expect $DBG_TOOLS_PATH/pull;;
			*)
				echo
		esac ;;
	82)
		dir=$(get_dir)
		case $dir in
			$DBG_ROOT_PATH)	
				cd $dir && lidbg_push;;
			$DBG_SYSTEM_DIR)
				read -p "输入提交到二进制仓库的说明文字：" descriptors
				depository_add_push $descriptors;;
			$RELEASE_REPOSITORY)
				cd $dir && expect $DBG_TOOLS_PATH/push;;
			*)
				echo
		esac ;;
	83)
		dir=$(get_dir)
		cd $dir && gitk & ;;
	84)
		dir=$(get_dir)
		cd $dir && git log --oneline;;
	85)
		dir=$(get_dir)
		cd $dir && git reset --hard && chmod 777 * -R;;
	86)
		dir=$(get_dir)
		cd $dir && nautilus .;;
	*)
		echo
	esac
}

