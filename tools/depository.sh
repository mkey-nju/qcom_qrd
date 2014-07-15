function depository_clean()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	git reset --hard
}

function depository_pull()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	expect $DBG_TOOLS_PATH/pull
}

function depository_add_push()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	git add .
	git add -f $UPDATA_BIN_DIR
	git commit -am $1
	expect $DBG_TOOLS_PATH/push
	gitk &
}

function depository_copy_lidbg()
{
	echo $FUNCNAME
	cp -r $DBG_OUT_PATH  $UPDATA_BIN_DIR
	cp -r $DBG_OUT_PATH/*.so  $UPDATA_BIN_DIR/hw
}

function depository_copy_basesystem()
{
	echo $FUNCNAME
	cp -r $DBG_SYSTEM_DIR/flyaudio/out/*  $UPDATA_BASESYSTEM_DIR
}


function depository_make_package()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	expect $DBG_TOOLS_PATH/make_package
}


function depository_menu()
{
if [[ $RELEASE_REPOSITORY != "" ]];then
	echo $RELEASE_REPOSITORY
	echo [41] copy lidbg out to depository
	echo [42] copy basesystem to depository
	echo [43] make package
fi
}

function depository_handle()
{
	cd $RELEASE_REPOSITORY
	case $1 in
	41)	
		depository_copy_lidbg;;
	42)
		depository_copy_basesystem;;
	43)
		depository_make_package;;
	*)
		echo
	esac
}


