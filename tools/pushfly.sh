
source ../build/env_entry.sh
expect $DBG_TOOLS_PATH/remount
adb  wait-for-device remount
#adb shell rm /flysystem/lib/out/*.ko
adb shell sync
adb push $DBG_OUT_PATH /flysystem/lib/out
. $DBG_TOOLS_PATH/pushfly_$DBG_SOC.sh
adb shell chmod 777 /flysystem/lib/out/*.ko
adb shell rm -r /system/lib/modules/out
adb shell sync
adb reboot
