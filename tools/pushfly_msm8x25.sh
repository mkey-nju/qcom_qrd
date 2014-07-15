
source ../build/env_entry.sh

adb push $DBG_OUT_PATH/camera.msm7627a.so /flysystem/lib/hw
adb push $DBG_OUT_PATH/gps.msm8625.so /flysystem/lib/hw
