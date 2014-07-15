
source ../build/env_entry.sh
expect $DBG_TOOLS_PATH/remount
adb  wait-for-device remount
#adb shell rm /system/lib/modules/out/*.ko
adb shell sync
adb push $DBG_OUT_PATH /system/lib/modules/out
adb shell chmod 777 /system/lib/modules/out/*.ko
adb shell rm -r /flysystem/lib/out
adb shell sync
adb reboot
