
source ../build/env_entry.sh
expect $DBG_TOOLS_PATH/root
adb shell insmod /system/lib/modules/out/lidbg_soc_msm8x25.ko
adb shell insmod /system/lib/modules/out/lidbg_msg.ko
adb shell insmod /system/lib/modules/out/lidbg_common.ko
adb shell insmod /system/lib/modules/out/lidbg_servicer.ko
adb shell insmod /system/lib/modules/out/lidbg_touch.ko
adb shell insmod /system/lib/modules/out/lidbg_key.ko
adb shell insmod /system/lib/modules/out/lidbg_i2c.ko
adb shell insmod /system/lib/modules/out/lidbg_io.ko
adb shell insmod /system/lib/modules/out/lidbg_ad.ko
adb shell insmod /system/lib/modules/out/lidbg_uart.ko
adb shell insmod /system/lib/modules/out/lidbg_main.ko
adb shell insmod /system/lib/modules/out/lidbg_fly_hal.ko

adb shell insmod /system/lib/modules/out/lidbg_ts_to_recov.ko
adb shell insmod /system/lib/modules/out/lidbg_ts_probe.ko
adb shell insmod /system/lib/modules/out/lidbg_fastboot.ko
adb shell insmod /system/lib/modules/out/lidbg_lpc.ko
adb shell insmod /system/lib/modules/out/lidbg_soc_devices.ko
adb shell insmod /system/lib/modules/out/lidbg_bpmsg.ko
adb shell insmod /system/lib/modules/out/lidbg_gps.ko
adb shell insmod /system/lib/modules/out/lidbg_videoin.ko
