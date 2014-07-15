source ../../../dbg_cfg.sh
echo $DBG_PLATFORM
if [ $DBG_PLATFORM = msm7627a ];then
rm ../out/camera.msm7627a.so
source ../build_cmn.sh
fi


