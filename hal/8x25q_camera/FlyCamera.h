#ifndef FLYCAMERA_HAL_H
#define FLYCAMERA_HAL_H
namespace android {
extern void FlyCameraStar();
extern void FlyCameraStop();
extern void FlyCameraRelease();
extern void FlyCameraGetInfo(mm_camera_ch_data_buf_t *Frame,QCameraHardwareInterface *mHalCamCtrl_1);
extern void FlyCameraNotSignalAtLastTime();
extern void FlyCameraThisIsFirstOpenAtDVD();
extern bool FlyCameraImageDownFindBlackLine();
extern bool FlyCameraFrameDisplayOrOutDisplay();

extern float FlyCameraflymFps;
extern char video_channel_status[10];

#define TW9912_IOC_MAGIC  'T'
#define COPY_TW9912_STATUS_REGISTER_0X01_4USER   _IO(TW9912_IOC_MAGIC, 1)
#define AGAIN_RESET_VIDEO_CONFIG_BEGIN   _IO(TW9912_IOC_MAGIC, 2)
#define TW9912_IOC_MAXNR 2
}; // namespace android

#endif
