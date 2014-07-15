
source ../build/env_entry.sh

cp -rf $DBG_OUT_PATH $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/lib/modules
cp $DBG_OUT_PATH/lidbg_servicer $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin
cp $DBG_OUT_PATH/lidbg_userver $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin
cp $DBG_OUT_PATH/gps.$DBG_PLATFORM.so $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/lib/hw

