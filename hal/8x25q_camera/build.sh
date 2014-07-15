source ../../../dbg_cfg.sh
echo $DBG_PLATFORM
if [ $DBG_PLATFORM = msm8625 ];then
rm ../out/camera.msm7627a.so
source ../build_cmn.sh
fi

