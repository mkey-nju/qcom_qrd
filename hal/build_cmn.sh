LOCATE_PATH=`pwd`
echo $LOCATE_PATH
source ../../dbg_cfg.sh
cd ../../build && source env_entry.sh && ./build_cfg.sh $DBG_SOC $BUILD_VERSION
cd $DBG_SYSTEM_DIR/&&source build/envsetup.sh&&choosecombo release $DBG_PLATFORM $SYSTEM_BUILD_TYPE
mmm $LOCATE_PATH -B
while :;do		
	echo
	echo -e "\033[41;20m 请输入字母 b ，再次编译 \033[0m"
	read Parameter0
	if [ $Parameter0 = b ];then
		mmm $LOCATE_PATH
	else 
	   exit
	fi
done
