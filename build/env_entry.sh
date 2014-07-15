#=======================================================================================
#	FileName    : 
#	Description : 
#       Date:         2010/04/27
#=======================================================================================

source ../dbg_cfg.sh

DBG_ROOT_PATH=`cd ../ && pwd`
DBG_BUILD_PATH=$DBG_ROOT_PATH/build
DBG_TOOLS_PATH=$DBG_ROOT_PATH/tools
DBG_OUT_PATH=$DBG_ROOT_PATH/out
DBG_CORE_PATH=$DBG_ROOT_PATH/core
DBG_SOC_PATH=$DBG_ROOT_PATH/soc
DBG_DRIVERS_PATH=$DBG_ROOT_PATH/drivers
DBG_HAL_PATH=$DBG_ROOT_PATH/hal
DBG_PLATFORM_DIR=$DBG_SOC_DIR/$DBG_PLATFORM


case "$DBG_PLATFORM" in
    msm7627a)
	BOARD_VERSION=V2
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=eng
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	DBG_SOC=msm8x25;;
    msm8625)
	BOARD_VERSION=V4
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=userdebug
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	UPDATA_BIN_DIR=$RELEASE_REPOSITORY/driver
	UPDATA_BASESYSTEM_DIR=$RELEASE_REPOSITORY/basesystem
	DBG_SOC=msm8x25;;
    msm8226)
	BOARD_VERSION=V1
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=eng
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	DBG_SOC=msm8x26;;
     su640)
	BOARD_VERSION=V1
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=eng
	DBG_KERNEL_SRC_DIR=/home/android/su640/system/kernel/lge/iproj
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ;;
esac



export DBG_ROOT_PATH
export DBG_TOOLS_PATH
export DBG_BUILD_PATH
export DBG_OUT_PATH
export DBG_CORE_PATH
export DBG_SOC_PATH
export DBG_DRIVERS_PATH
export DBG_PLATFORM_DIR
export RELEASE_REPOSITORY

export DBG_SOC
export DBG_PLATFORM
export DBG_SYSTEM_DIR
export UPDATA_BIN_DIR
export UPDATA_BASESYSTEM_DIR
export DBG_KERNEL_SRC_DIR
export DBG_KERNEL_OBJ_DIR
export DBG_CROSS_COMPILE
export BOARD_VERSION
export BUILD_VERSION

