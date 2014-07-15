source ../dbg_cfg.sh
source ../soc/$DBG_SOC/conf/soc_select
LOCATE_PATH=`pwd`
clear
cd ../build && source env_entry.sh && ./build_cfg.sh  $DBG_SOC $BUILD_VERSION
cd $DBG_SYSTEM_DIR/&&source build/envsetup.sh&&choosecombo release $DBG_PLATFORM $SYSTEM_BUILD_TYPE && mmm $DBG_HAL_PATH -B

