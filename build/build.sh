
source ./env_entry.sh
./build_cfg.sh $DBG_SOC $BOARD_VERSION
mkdir -p $DBG_OUT_PATH

build_log=$DBG_OUT_PATH/build_time.conf
hostname > $build_log
date >> $build_log
echo $DBG_PLATFORM >> $build_log
echo $BOARD_VERSION >> $build_log
echo $BUILD_VERSION >> $build_log
git log --oneline | sed -n '1,10p' >> $build_log

#find ../ -name "Module.symvers" -exec rm -rf {} \;
./make_all.sh  && . $DBG_BUILD_PATH/copy2out.sh 


