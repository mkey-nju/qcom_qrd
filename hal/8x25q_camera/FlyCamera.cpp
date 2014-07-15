#define ALOG_TAG "FlyCamera.cpp"
#include <utils/Log.h>
#include <utils/threads.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "QCameraHAL.h"
#include "QCameraHWI.h"
#include "FlyCamera.h"
#include <genlock.h>
#include <gralloc_priv.h>
#include <linux/msm_mdp.h>
#include <cutils/properties.h>
#include "../inc/lidbg_servicer.h"

#if 1
#define DEBUGLOG ALOGE
#else
#define DEBUGLOG do{}while(0)
#endif

#define ERR_LOG LIDBG_PRINT
//ERR_LOG("Flyvideo Error:...");

#define BCAR_BLACK_VALUE_UPERR_LIMIT /*0x42*/ 0x42//如果有对视频色彩的亮度或颜色有修改 这个值需要重新取值
namespace android {

	typedef struct
	{
		unsigned char reg;
		unsigned char reg_val;
		bool sta;//true is find black line;
		bool flag;//true is neet again find the black line;
		bool this_is_first_open;//true is first open AND is open DVD
	}TW9912Info;

	typedef struct
	{
		unsigned int BlackCoordinateX;//第一次发现黑点的坐标
		unsigned int BlackCoordinateY;
		unsigned int BlackFreamCount;//同个x坐标下，帧数统计
		bool ThisIsFirstFind;//是否是第一次发现，区别在于，若是第一次寻找黑点，需逐个遍历，否这直接判断对应的纵行。
		unsigned int BegingFindBlackFreamCount;//第一次发现黑点的坐标
	}SplitScreenInfo;

	char video_channel_status[10]="0";
	float FlyCameraflymFps = 0;

	static bool ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = false;//在下一数据帧率到来时是否再次执行 寻找黑线的动作 ture 不用再找
	static char video_format[10]="1";
	static char video_show_status[10]="0";
	static mm_camera_ch_data_buf_t *frame;
	static QCameraHardwareInterface *mHalCamCtrl;
	static unsigned int open_dev_fail_count=0;
	static unsigned int global_fram_at_one_sec_count = 0;
	static unsigned int Longitudinal_last_fream_count =9;
	static SplitScreenInfo LongitudinalInformationRemember={300,0,0,true,0};
	static bool global_need_rePreview = false;
	static pthread_t thread_DetermineImageSplitScreenID;
	static bool DetermineImageSplitScreen_do_not_or_yes = true;
	static int global_fram_count = 0;
	static nsecs_t Astern_last_time = 0;
	static nsecs_t DVDorAUX_last_time = 0;
	static bool globao_mFlyPreviewStatus;
	static unsigned int rePreview_count = 0;
	static bool global_Fream_is_first = true;
	static int flymFrameCount;
	static int flymLastFrameCount = 0;
	static nsecs_t flymLastFpsTime = 0;
  static nsecs_t flynow;
  static nsecs_t flydiff;
  static int global_tw9912_file_fd = NULL;
  static int global_fream_give_up =0;
	static unsigned int global_last_critical_point = 50;
  static unsigned int global_last_bottom_critical_point = 435;

void *CameraRestartPreviewThread(void *mHalCamCtrl1);

void FlyCameraStar()
{
TW9912Info tw9912_info;
	globao_mFlyPreviewStatus = 1;
	global_fream_give_up = 0;
	//global_last_critical_point = 50;
	//global_last_bottom_critical_point = 435;
	DEBUGLOG("Flyvideo-mFlyPreviewStatus =%d\n",globao_mFlyPreviewStatus);
	ERR_LOG("Flyvideo :Camera Start Preview\n");
	if(global_tw9912_file_fd == NULL || global_tw9912_file_fd == -1)global_tw9912_file_fd = open("/dev/tw9912config",O_RDWR);
	if(global_tw9912_file_fd ==-1)
		{
		DEBUGLOG("Flyvideo-:Error FlyCameraStar() tw9912config faild\n");
		ERR_LOG("Flyvideo Error:FlyCameraStar() tw9912config faild\n");
		}

  property_get("fly.video.channel.status",video_channel_status,"1");//1:DVD 2:AUX 3:Astren
  if(video_channel_status[0] == '3' && DetermineImageSplitScreen_do_not_or_yes == false)//在上次打开为找到黑线，这次打开再运行进行黑线寻找
		{//图像下移寻找黑线的动作未找到，下一次的倒车再找一遍
			DEBUGLOG("Flyvideo-:在上次打开倒车中，未找到黑线，再找一次\n");
			ERR_LOG("Flyvideo :At last open camera find black line fail,now again find that\n");
			ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = false;
			read(global_tw9912_file_fd, (void *)(&tw9912_info),sizeof(TW9912Info));
			tw9912_info.sta = false;
			tw9912_info.flag = true;
			tw9912_info.reg = 0x08;
			tw9912_info.reg_val = 0x17;
			write(global_tw9912_file_fd, (const void *)(&tw9912_info),sizeof(TW9912Info));
		}
}
void FlyCameraStop()
{
rePreview_count = 0;
ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = false; 
open_dev_fail_count = 0;
global_fram_at_one_sec_count = 0;
LongitudinalInformationRemember.ThisIsFirstFind = true;
LongitudinalInformationRemember.BegingFindBlackFreamCount = 0;
DetermineImageSplitScreen_do_not_or_yes = true;//at next preview ,allow run function DetermineImageSplitScreen at astren
globao_mFlyPreviewStatus = 0;
global_Fream_is_first = true;
//close(global_tw9912_file_fd);
DEBUGLOG("Flyvideo-mFlyPreviewStatus =%d\n",globao_mFlyPreviewStatus);
ERR_LOG("Flyvideo :Camera Preview stop\n");
}
void FlyCameraRelease()
{
Longitudinal_last_fream_count =9;//判断纵向分屏的时间标签
}

void FlyCameraGetInfo(mm_camera_ch_data_buf_t *Frame,QCameraHardwareInterface *mHalCamCtrl_1)
{
    property_get("fly.video.show.status",video_show_status,"1");// 1 is show normal, 0 is show black
    property_get("fly.video.channel.status",video_channel_status,"1");//1:DVD 2:AUX 3:Astren
    property_get("tcc.fly.vin.pal",video_format,"1");//pal_enabel
		/*
		tcc.fly.vin.pal:
											DVD : 	NTSC	PAL
																1		2
								CAM or AUX: 	NTSC	PAL
																3			4					
		*/
		frame = Frame;
		mHalCamCtrl = mHalCamCtrl_1;
    flymFrameCount++;
    flynow = systemTime();
    flydiff = flynow - flymLastFpsTime;
    if (flydiff > ms2ns(250)) {
        FlyCameraflymFps =  ((flymFrameCount - flymLastFrameCount) * float(s2ns(1))) / flydiff;
        //DEBUGLOG("Preview Frames Per Second fly: %.4f", FlyCameraflymFps);
        flymLastFpsTime = flynow;
        flymLastFrameCount = flymFrameCount;
    }

}
static int ToFindBlackLineAndSetTheTw9912VerticalDelayRegister(mm_camera_ch_data_buf_t *frame);
bool FlyCameraImageDownFindBlackLine()//下移动图像 寻找黑线
{
		if(video_channel_status[0] == '3' && ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok == false)//onle at Astren and not PAL
			{
				if(ToFindBlackLineAndSetTheTw9912VerticalDelayRegister(frame) == 1)
					{
					DEBUGLOG("Flyvideo-注意：图像静帧一帧");
					ERR_LOG("Flyvideo :Find black line ,frame give up\n");
					return 1;
					}
			}
return 0;
}
void FlyCameraThisIsFirstOpenAtDVD()//第一次打开DVD做一次重新的preview
{
	if(video_channel_status[0] == '1' && global_Fream_is_first == true)
			{
						 //int file_fd;
						 TW9912Info tw9912_info;
						// file_fd = open("/dev/tw9912config",O_RDWR);
						 if(global_tw9912_file_fd ==-1)
						 {
							       DEBUGLOG("Flyvideo-错误，打开设备节点文件“/dev/tw9912config”失败");
											ERR_LOG("Flyvideo Error:Open tw9912config node fail\n");
							      // return -1;
											goto OPEN_ERR;
						 }
						 read(global_tw9912_file_fd, (void *)(&tw9912_info),sizeof(TW9912Info));

						if(global_need_rePreview == false )
						{
							global_need_rePreview = true;
							DEBUGLOG("Flyvideo-xx: 16：9视频分屏");
							if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
							{DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览失败！DVD\n");ERR_LOG("Flyvideo Error:CameraRestartPreviewThread faild\n");}
							//else
							//DEBUGLOG("Flyvideo-发现分屏 DVD\n");
						}
						/*
						 if(tw9912_info.this_is_first_open == true)
						 {//这里的调用最好用线程方式调用
							       DEBUGLOG("Flyvideo-x:第一次打开DVD\n");
											ERR_LOG("Flyvideo :This is first open DVD and now restart Preview\n");
							       mHalCamCtrl->stopPreview();
							      	usleep(1000);
							       mHalCamCtrl->startPreview();
							       //      sleep(1);
						 }*/
						 tw9912_info.this_is_first_open = false;
						 write(global_tw9912_file_fd, (const void *)(&tw9912_info),sizeof(TW9912Info));
						 //close(file_fd);
							OPEN_ERR:
						 global_Fream_is_first = false;
			}
}
void FlyCameraNotSignalAtLastTime()//在本次的倒车时候 ，开始还没检测到视频输入信号，后来信号来啦，要重新配置下视频芯片和repreview
{
	if(video_show_status[0] == '0' && rePreview_count > 20)//应该调用 视频检测函数来代替 延时变量rePreview_count++ 
		{//发现黑屏 且 视频在上次打开没有视频源输入
			int arg = 0;
			unsigned int cmd;
			cmd = AGAIN_RESET_VIDEO_CONFIG_BEGIN;
			if (ioctl(global_tw9912_file_fd,cmd, &arg) < 0)
		        {
				DEBUGLOG("Flyvideo-: ioctl AGAIN_RESET_VIDEO_CONFIG_BEGIN 4 tw9912 node  fail\n");
				ERR_LOG("Flyvideo Error:ioctl tw9912config node fail\n");
			}
			if(global_need_rePreview == false )
			{
					global_need_rePreview = true;
					DEBUGLOG("Flyvideo-上次的open是碰到视频不稳定，再次配置chip后的再次预览");
					ERR_LOG("Flyvideo :at open camera not video signal,so,again setting chip config\n");
					if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
						{DEBUGLOG("Flyvideo-创建线程重新预览失败！DVD\n");
						ERR_LOG("Flyvideo Error:CameraRestartPreviewThread fail\n");}
					//else
					//	DEBUGLOG("Flyvideo-发现分屏 DVD\n");
			}
			rePreview_count = 0;
		}
		else if(video_show_status[0] == '0')
		{
			rePreview_count++;
			if(rePreview_count >10000)rePreview_count = 21;
		}
}
void *CameraRestartPreviewThread(void *mHalCamCtrl1)
{
	QCameraHardwareInterface *mHalCamCtrl = (QCameraHardwareInterface *)mHalCamCtrl1;

	if(globao_mFlyPreviewStatus == 1)
	{
		DEBUGLOG("Flyvideo-:stopPreview()-->\n");
		ERR_LOG("Flyvideo :stop preview\n");
		mHalCamCtrl->stopPreview();
		//	sleep(1);
		usleep(1000);
		DEBUGLOG("Flyvideo-:startPreview()-->\n");
		ERR_LOG("Flyvideo :start preview\n");
		mHalCamCtrl->startPreview();
		//	sleep(1);
		usleep(500000);//500 ms
	}
	else
	{
	DEBUGLOG("Flyvideo-发现分屏需要重新preview，但是视频HAL调用啦stop。\n");
	}

	LongitudinalInformationRemember.ThisIsFirstFind = true;
	global_need_rePreview = false;
    return NULL;
}

static bool ToFindBlackLine(mm_camera_ch_data_buf_t *frame)//寻找是否真的存在黑线（分屏判断确定）
{
	unsigned char *piont_y,*piont_y_last;
	unsigned int i,j,i_last=0;
	unsigned int black_count=0;
		for(i=0;i<719;i++)//第几列
		{
				piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+ i);//第一行
				if(!i) piont_y_last = piont_y;

				//if( (*piont_y) == (*piont_y_last) && (*piont_y) < 0x25)//TW9912 黑屏数据是 0x20
				//if( (*piont_y) == (*piont_y_last) && (*piont_y) < 0x2f)//TW9912 黑屏数据是 0x20
				if(/*(*piont_y) >= 0x20 &&*/ (*piont_y) <= BCAR_BLACK_VALUE_UPERR_LIMIT/*0x42*//*0x35*/ )
				{
					black_count++;
					if( (719-i+black_count)< 710)//一定无法达到700个点的要求，没必要再执行下去。
					return 0;
					if( black_count > 710)//“黑色"数据大于700认为是黑线出现
					{
						//FlagBlack(frame);
						DEBUGLOG("Flyvideo-：第一行黑色行，成功腾出,在这行黑色数据的个数 = %d 个,其中一个黑色数据是0x%.2x",black_count,*piont_y);
						return 1;
					}
				}
				//else return 0;
				i_last =i;
				piont_y_last = piont_y;
		}
	return 0;
}
/*
static void FlagBlack(mm_camera_ch_data_buf_t *frame)//填充白色
{
int i,j;
static int NumberFlag=0;
for(j=100;j<120;j++)
	for(i=NumberFlag*100;i<=(NumberFlag*100+20);i+=4)
	{
	*(unsigned long *)(frame->def.frame->buffer+frame->def.frame->y_off+j*720+i) |=0xffffffff;
	}
NumberFlag++;
if(NumberFlag>7)NumberFlag=0;
}
*/
static int ToFindBlackLineAndSetTheTw9912VerticalDelayRegister(mm_camera_ch_data_buf_t *frame)
{
	//int file_fd;
	TW9912Info tw9912_info;
	//file_fd = open("/dev/tw9912config",O_RDWR);
		if(global_tw9912_file_fd ==-1)
		{
				if(open_dev_fail_count == 0)
				{
						DEBUGLOG("Flyvideo-错误x，打开设备节点文件“/dev/tw9912config”失败");
						ERR_LOG("Flyvideo Error:open tw9912config fail\n");
						//ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = true;
						DEBUGLOG("Flyvideo-警告：重要！！由于节点/dev/tw9912config打开失败，相关配置无法完成，分屏判断将失效，请检查该节点的读写权限（>606）");
						DetermineImageSplitScreen_do_not_or_yes = false;
						open_dev_fail_count ++;
						goto OPEN_ERR;
						//return -1;
				}
				else if( open_dev_fail_count >60 )
				{
						open_dev_fail_count = 1;
						if(global_tw9912_file_fd ==-1)
						{
							DEBUGLOG("Flyvideo-错误`，打开设备节点文件“/dev/tw9912config”失败");
							//ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = true;
							DEBUGLOG("Flyvideo-警告：重要！！由于节点/dev/tw9912config打开失败，相关配置无法完成，分屏判断将失效，请检查该节点的读写权限（>606）");
							DetermineImageSplitScreen_do_not_or_yes = false;
							goto OPEN_ERR;
							//return -1;
						}
				}
				else
				{
					open_dev_fail_count ++;
					goto OPEN_ERR;
				}
		}

	read(global_tw9912_file_fd, (void *)(&tw9912_info),sizeof(TW9912Info));
	if(tw9912_info.flag == true)
	{
		if(ToFindBlackLine(frame) ==1)//找到啦黑色数据行
		{
			tw9912_info.sta = true;
			tw9912_info.flag = false;
			write(global_tw9912_file_fd, (const void *)(&tw9912_info),sizeof(TW9912Info));
		DEBUGLOG("Flyvideo-找到了黑色数据行，图像下移策略不需再执行");
		ERR_LOG("Flyvideo :first black line find\n");
		ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = true;
		DetermineImageSplitScreen_do_not_or_yes = true;
		}
		else//还没找到黑色数据行 图像下移一行
		{
			if(tw9912_info.reg_val > 0x10 && tw9912_info.reg_val <= 0x17)
			{
				tw9912_info.reg = 0x08;
				tw9912_info.reg_val -= 1;
				write(global_tw9912_file_fd, (const void *)(&tw9912_info),sizeof(TW9912Info));
				DEBUGLOG("Flyvideo-黑色数据行还未找到，图像下移一个像素点");
				ERR_LOG("Flyvideo :first black line finding..\n");
			}
			else
			{
				DEBUGLOG("Flyvideo-警告：图像顶部腾出的黑线还未找到，但是图像下移数据的设置已经达到下限，不能再设置更小");
				ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = true;
				DEBUGLOG("Flyvideo-警告：重要！！由于腾出黑线的操作无法完成，分屏判断将失效");
				ERR_LOG("Flyvideo Error:important find first black line faild\n");
				DetermineImageSplitScreen_do_not_or_yes = false;
			}
		return 1;
		}

	}
	else
	{
		DEBUGLOG("Flyvideo-:无需再“腾”顶部的黑线");
		ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = true;
	}
	//close(file_fd);
OPEN_ERR:
return 0;
}
static bool VideoItselfBlackJudge(mm_camera_ch_data_buf_t *frame)//判断视频是否是本身是黑色的数据
{
	unsigned int black_count;
	int i =0 ,j =0;
	unsigned char *piont_y;

	black_count = 0;
	for(j=60;j<400;j++)
		{i++;
			piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*j + i);//取值 正方形 斜线查找
			if(*piont_y <= 0x20)//找到啦一个黑色点
				black_count ++;

			if( (400 - j + black_count) < 170)//一定无法达到470个点的要求，没必要再执行下去。
						continue;

			if(black_count>170)
				{
					//DEBUGLOG("Flyvideo-:黑");
					return 1;//这帧本身是黑色
				}
		}

	for(j=400;j>60;j--)
		{i++;
			piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*j + i);//取值 正方形 斜线查找
			if(*piont_y <= 0x20)//找到啦一个黑色点
				black_count ++;

			if( (400 - j + black_count) < 280)//一定无法达到470个点的要求，没必要再执行下去。
						continue;

			if(black_count>280)
				{
					//DEBUGLOG("Flyvideo-:黑");
					return 1;//这帧本身是黑色
				}
		}
return 0;
}
/*
static bool BlackJudge(mm_camera_ch_data_buf_t *frame)
{
unsigned char *piont_y,*piont_y_last;
unsigned int i,j,i_last=0;
unsigned int black_count=0;
//FlagBlack(frame);
//网格判断
//	for(j=477;j<479;j++)//第几行
//	{
		for(i=0;i<719;i++)//第几列
		{
				piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+475*720+ i);
				//piont_y_last = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+j*720+ i_last);
				if(!i) piont_y_last = piont_y;

				//if( (*piont_y) == (*piont_y_last) && (*piont_y) < 0x25)//TW9912 黑屏数据是 0x20
				if( (*piont_y) == (*piont_y_last) && (*piont_y) < 0x2f)//TW9912 黑屏数据是 0x20
				{
					black_count++;
					if( (719-i+black_count)< 280)//一定无法达到700个点的要求，没必要再执行下去。
					return 0;
					if( black_count > 280)//“黑色"数据大于700认为是黑屏
					{
						//FlagBlack(frame);
						DEBUGLOG("Flyvideo-：确定倒车视频已断开,发现的黑色数据个数= %d 个,其中一个黑色数据是0x%.2x",black_count,*piont_y);
						return 1;
					}
				}
				//else return 0;
				i_last =i;
				piont_y_last = piont_y;
		}
	//}
return 0;
}
*/
static bool RowsOfDataTraversingTheFrameToFindTheBlackLineForDVDorAUX(mm_camera_ch_data_buf_t *frame)
{
	unsigned char *piont_y;
	unsigned int i = 0,jj =0;
	unsigned int count;
	unsigned int find_line = 477;//NTSC
	if(video_format[0] == '4' || video_format[0] == '2')
	{
	//DEBUGLOG("Flyvideo-:视频制式是:PAL，寻找范围是：9～562");
	find_line = 562;//PAL
	}
	//else DEBUGLOG("Flyvideo-:视频制式是:NTSC，寻找范围是：9～477");
			DEBUGLOG("Flyvideo-可能发生分屏，寻找黑边中。。。");
			ERR_LOG("Flyvideo :Find black line..\n");
			for(i=9;i<find_line;i++)//某列下的第几行，找 一个点 看数据是否是黑色；前10行和后10行放弃找，正常情况下前3行是黑色的数据
			{
				piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + 300);//在第300列下的每行找黑点
				if(*piont_y <= 0x43)//到此，在某列下的某行，找到啦一个黑点，接下来对这一行，遍历700个点，看这行是否确实都是黑色数据
				//if((*piont_y) >= 0x20 && (*piont_y) <= 0x35 )
				{
					for(jj=0;jj<719;jj++)//在一行的遍历
					{
						piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + jj);
						if(*piont_y <= 0x43)
						//if((*piont_y) >= 0x20 && (*piont_y) <= 0x35 )
							count++;//黑点
						if( (719-jj+count)< 715)//一定无法达到700个点的要求，没必要再执行下去。
						{//如果剩下的（715-jj）加上以发现的（count）已经小于700，将结束循环
							goto Break_The_For;
						}
					}
					Break_The_For:
					if(count >= 715) goto Break_The_Func;//当这行的黑色数据确实有700个，跳出整个遍历，返回分屏信息。
					else count =0;
				}
			}
//DEBUGLOG("Astern：经确认未发生黑屏");
DEBUGLOG("Flyvideo-找不到黑边");
ERR_LOG("Flyvideo :black line not find, so frame is ok\n");
return 0;
Break_The_Func:
DEBUGLOG("Flyvideo-：%d行分屏，数据%d个,黑色数据0x%.2x",i,count,*piont_y);
return 1;//确定这帧是出现啦分屏
}
static bool RowsOfDataTraversingTheFrameToFindTheBlackLine(mm_camera_ch_data_buf_t *frame)
{
	unsigned char *piont_y;
	unsigned int i = 0,jj =0;
	unsigned int count;
	unsigned int find_line = 477;//NTSC
	if(video_format[0] == '4' || video_format[0] == '2')
	{
	//DEBUGLOG("Flyvideo-:视频制式是:PAL，寻找范围是：9～562");
	find_line = 562;//PAL
	}
	//else DEBUGLOG("Flyvideo-:视频制式是:NTSC，寻找范围是：9～477");
	DEBUGLOG("Flyvideo-可能发生分屏，寻找黑边中。。。");
	ERR_LOG("Flyvideo :Find black line..\n");
			for(i=9;i<find_line;i++)//某列下的第几行，找 一个点 看数据是否是黑色；前10行和后10行放弃找，正常情况下前3行是黑色的数据
			{
				piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + 300);//在第300列下的每行找黑点
				//if(*piont_y <= 0x20)//到此，在某列下的某行，找到啦一个黑点，接下来对这一行，遍历700个点，看这行是否确实都是黑色数据
				//if((*piont_y) >= 0x20 && (*piont_y) <= 0x35 )
				if(/*(*piont_y) >= 0x20 &&*/ (*piont_y) <= BCAR_BLACK_VALUE_UPERR_LIMIT /*0x42*//*0x35*/ )
				{
					for(jj=0;jj<719;jj++)//在一行的遍历
					{
						piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + jj);
						//if(*piont_y <= 0x20)
						//if((*piont_y) >= 0x20 && (*piont_y) <= 0x35 )
						if(/*(*piont_y) >= 0x20 &&*/ (*piont_y) <= BCAR_BLACK_VALUE_UPERR_LIMIT /*0x42*//*0x35*/ )
							count++;//黑点
						if( (719-jj+count)< 715)//一定无法达到700个点的要求，没必要再执行下去。
						{//如果剩下的（715-jj）加上以发现的（count）已经小于700，将结束循环
							goto Break_The_For;
						}
					}
					Break_The_For:
					if(count >= 715) goto Break_The_Func;//当这行的黑色数据确实有700个，跳出整个遍历，返回分屏信息。
					else count =0;
				}
			}
//DEBUGLOG("Astern：经确认未发生黑屏");
DEBUGLOG("Flyvideo-找不到黑边");
ERR_LOG("Flyvideo :black line not find,so frame is ok\n");
return 0;
Break_The_Func:
DEBUGLOG("Flyvideo-：在第 %d 行确定发生啦分屏,数据=%d 个,黑色=0x%.2x",i,count,*piont_y);
return 1;//确定这帧是出现啦分屏
}
//Determine whether the split-screen
static bool DetermineImageSplitScreen_Longitudinal(mm_camera_ch_data_buf_t *frame,QCameraHardwareInterface *mHalCamCtrl)
{//纵向分屏判断
	int i =0 ,j = 30,j_end = 690;
	unsigned char *piont_y;
	unsigned int dete_count = 0;
	Longitudinal_last_fream_count ++;
	if(Longitudinal_last_fream_count > 0xfffe) Longitudinal_last_fream_count = 9;
	if( Longitudinal_last_fream_count < 4)//时间过滤，防止短时间内，连续的出现preview操作
	{//DEBUGLOG("Flyvideo-:T");
			return 0;
	}
	if(LongitudinalInformationRemember.ThisIsFirstFind == false)
	{//连续5帧还没做完判断，重新定位黑边坐标
		LongitudinalInformationRemember.BegingFindBlackFreamCount++;
		if(LongitudinalInformationRemember.BegingFindBlackFreamCount > 6 || (6 - LongitudinalInformationRemember.BegingFindBlackFreamCount +LongitudinalInformationRemember.BlackFreamCount ) < 5 )//已经超过统计数，或者 剩下的加已经统计的少于10个
		{DEBUGLOG("Flyvideo-:纵向分屏累计清零");
			ERR_LOG("Flyvideo :longitudinal count clrean\n");
			LongitudinalInformationRemember.BegingFindBlackFreamCount = 0;
			LongitudinalInformationRemember.ThisIsFirstFind = true;
		}
	}
	if(LongitudinalInformationRemember.ThisIsFirstFind == false)//如果已经定位啦黑色边,不再做横向遍历查找黑点坐标
	{//黑色行位置已经确定
		j = LongitudinalInformationRemember.BlackCoordinateX;
		j_end = j;
		dete_count = 0 ;
			for(i=60;i<400;i++)
			{
				piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + j);
				if((*piont_y) <= 0x20)
				{
					dete_count++;
					if( (400 - i + dete_count) < 330)//一定无法达到400个点的要求，没必要再执行下去。
						{
							dete_count = 0;
							goto THE_FOR;//跳到下一个黑点对应的列统计。
						}
					if(dete_count > 330 && VideoItselfBlackJudge(frame) == 1)//提前一帧判断是否是黑色数据帧
						return 0;//未发生分屏
					if(dete_count > 330)//在这个纵向行黑色点个数达到啦设置值,且这帧本身不是黑色屏
					{
						dete_count = 0;
						Longitudinal_last_fream_count = 0;//时间标签更新
					//LOGE("DetermineImageSplitScreen:buf[%d]= 0x%.2x\n",((180*j) + i),(*piont_crcb) );
						if(LongitudinalInformationRemember.ThisIsFirstFind == true)//这里是第一次找到黑色点
						{
							LongitudinalInformationRemember.ThisIsFirstFind = false;//定位啦黑色边
							LongitudinalInformationRemember.BlackCoordinateX = j;//记下这一纵行
							LongitudinalInformationRemember.BlackFreamCount = 1;
						}
						else
						{
							LongitudinalInformationRemember.BlackFreamCount ++;
						}
						DEBUGLOG("Flyvideo-:第%d纵行,累计帧=%d\n",j,LongitudinalInformationRemember.BlackFreamCount);
						if(LongitudinalInformationRemember.BlackFreamCount  >250)//5sec
						{
							global_need_rePreview = false;//防止CameraRestartPreviewThread阻塞导致，global_need_rePreview无法赋值成true；
							LongitudinalInformationRemember.ThisIsFirstFind = true;
						}
						if(global_need_rePreview == false )
						{
								if(LongitudinalInformationRemember.BlackFreamCount >= 5) //发现连续的15帧中有 14 帧就重新preview
								{
									global_need_rePreview = true;
									LongitudinalInformationRemember.BlackFreamCount = 0;
									#if 0
											DEBUGLOG("Flyvideo-:纵向分屏,stopPreview()-->\n");
											mHalCamCtrl->stopPreview();
											DEBUGLOG("Flyvideo-:纵向分屏,startPreview()-->\n");
											mHalCamCtrl->startPreview();
									#else
											if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
												{DEBUGLOG("Flyvideo-:纵向分屏，创建线程重新预览失败！\n");
												ERR_LOG("Flyvideo Error:CameraRestartPreviewThread faild\n");}
											else
												DEBUGLOG("Flyvideo-:纵向分屏，创建线程重新预览成功！\n");
									#endif
									global_need_rePreview = false;
									LongitudinalInformationRemember.ThisIsFirstFind = true;
									LongitudinalInformationRemember.BegingFindBlackFreamCount = 0;
									return 1;//返回分屏信息
								}
								else
								return 0;
						}
					goto BREAK_THE;
					}
				}
			}
		THE_FOR:
		;
	}
	else
	{//黑色行位置还未确定
		for(;j<=j_end;j++)
			{
				piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*300 + j);//在第300行，横向扫描
				if(*piont_y <= 0x20)//找到啦一个黑色点
				{
					dete_count = 0 ;
					for(i=60;i<400;i++)
					{
						piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + j);
						if((*piont_y) <= 0x20)
						{
							dete_count++;
							if( (400 - i + dete_count) < 330)//一定无法达到400个点的要求，没必要再执行下去。
								{
									dete_count = 0;
									goto THE_FOR_2;//跳到下一个黑点对应的列统计。
								}
							if(dete_count > 330 && VideoItselfBlackJudge(frame) == 1)//提前一帧判断是否是黑色数据帧
								return 0;//未发生分屏
							if(dete_count > 330)//在这个纵向行黑色点个数达到啦设置值,且这帧本身不是黑色屏
							{
								dete_count = 0;
								Longitudinal_last_fream_count = 0;//时间标签更新
							//LOGE("DetermineImageSplitScreen:buf[%d]= 0x%.2x\n",((180*j) + i),(*piont_crcb) );
								if(LongitudinalInformationRemember.ThisIsFirstFind == true)//这里是第一次找到黑色点
								{
									LongitudinalInformationRemember.ThisIsFirstFind = false;//定位啦黑色边
									LongitudinalInformationRemember.BlackCoordinateX = j;//记下这一纵行
									LongitudinalInformationRemember.BlackFreamCount = 1;
								}
								else
								{
									LongitudinalInformationRemember.BlackFreamCount ++;
								}
								DEBUGLOG("Flyvideo-:第%d纵行,累计帧=%d\n",j,LongitudinalInformationRemember.BlackFreamCount);
								if(LongitudinalInformationRemember.BlackFreamCount  >250)//5sec
								{
									global_need_rePreview = false;//防止CameraRestartPreviewThread阻塞导致，global_need_rePreview无法赋值成true；
									LongitudinalInformationRemember.ThisIsFirstFind = true;
								}
								if(global_need_rePreview == false )
								{
									if(LongitudinalInformationRemember.BlackFreamCount >= 5) //发现连续的15帧中有 14 帧就重新preview
									{
										global_need_rePreview = true;
										LongitudinalInformationRemember.BlackFreamCount = 0;
										#if 0
												DEBUGLOG("Flyvideo-:纵向分屏,stopPreview()-->\n");
												mHalCamCtrl->stopPreview();
												DEBUGLOG("Flyvideo-:纵向分屏,startPreview()-->\n");
												mHalCamCtrl->startPreview();
										#else
												if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
													{DEBUGLOG("Flyvideo-:纵向分屏，创建线程重新预览失败！\n");
													ERR_LOG("Flyvideo Error:CameraRestartPreviewThread faild\n");}
												else
													{DEBUGLOG("Flyvideo-:纵向分屏\n");
													ERR_LOG("Flyvideo :Longitudinal spilt screen is happen\n");}
										#endif
										LongitudinalInformationRemember.ThisIsFirstFind = true;
										LongitudinalInformationRemember.BegingFindBlackFreamCount = 0;
										return 1;//返回分屏信息
									}
								}
							goto BREAK_THE;
							}
						}

					}
					THE_FOR_2:
					;
				}
			}
		}
BREAK_THE:
return 0;//未发生分屏
}
static void WriteDataToFrameBuffer(mm_camera_ch_data_buf_t *frame, unsigned int at_line_write, unsigned int start, unsigned int len)
{//写黑色数据到视频缓存
	memset((void *)(frame->def.frame->buffer+frame->def.frame->y_off+720*at_line_write + start),0xff,len);

}
static unsigned int LookingForABlackCriticalLine(mm_camera_ch_data_buf_t *frame,unsigned int star,unsigned int len,unsigned int down_or_up)
{
unsigned char *piont_y;
unsigned int line_count,jj,count=0;
unsigned int temp;
	if(down_or_up)//向下寻找
	{
			for(line_count = star ;line_count < star+len ; line_count ++)
					 {
							piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+ line_count*720 + 300);
								if(*piont_y <= 0x43)//发现一个黑点
								{
									//DEBUGLOG("Flyvideo-xx: 在 %d ,数%d,数据是%d\n",line_count,count,*piont_y);
									count = 0;
									for(jj=0;jj<700;jj++)//在一行的遍历 中部取值判断
									{
										piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*line_count + jj);
										if(*piont_y <= 0x30)
											count++;//黑点
										//else DEBUGLOG("Flyvideo-a:0x%.2x jj = %d line_count = %d count =%d\n",*piont_y,jj,line_count,count);
										if( (700-jj+count) < 690)//一定无法达到200个点的要求，没必要再执行下去。
										{//如果剩下的（600-jj）加上已经发现的（count）已经小于200，将结束循环
											//DEBUGLOG("Flyvideo-xx: 在 %d 行的黑点累计数达不到要求提前结束判断\n",line_count);
											goto Break_The_For;
										}
									}
									Break_The_For:
									if(count >= 650) goto GO_H_NEXT;//当这行的部分黑色数据确实有190个，跳出整个遍历
									else {
												//DEBUGLOG("Flyvideo-xx: 找到啦顶部的视频临界行在 %d 行，cont = %d\n",line_count,count);
												return line_count;
												}
							 }
							 else return line_count;
						GO_H_NEXT:
						;
					 }
		//DEBUGLOG("Flyvideo-xx: 寻找临界行 超出设定（%d）寻找的范围\n",star+len);
	}
	else
	{
		 for(line_count = star ;line_count > star - len ; line_count --)
				 {

						piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+ line_count*720 + 300);
							if(*piont_y <= 0x43)//发现一个黑点
							{//DEBUGLOG("Flyvideo-xx: 在 %d 行发现一个黑点\n",line_count);
								count = 0;
								for(jj=0;jj<700;jj++)//在一行的遍历 中部取值判断
								{
									piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*line_count + jj);
									if(*piont_y <= 0x20)
									//if((*piont_y) >= 0x20 && (*piont_y) <= 0x35 )
										count++;//黑点
									if( (700-jj+count)< 690)//一定无法达到200个点的要求，没必要再执行下去。
									{//如果剩下的（600-jj）加上已经发现的（count）已经小于200，将结束循环
										//DEBUGLOG("Flyvideo-xx: 在 %d 行的黑点累计数达不到要求提前结束判断\n",line_count);
										goto Break_The_For_2;
									}
								}
								Break_The_For_2:
								if(count >= 650) goto GO_H_NEXT_2;//当这行的部分黑色数据确实有190个，跳出整个遍历
								else { count =0; return  line_count;}
						 }
						 else return line_count;
					GO_H_NEXT_2:
					;
				 }
		//DEBUGLOG("Flyvideo-xx: 寻找临界行 超出设定（%d）寻找的范围\n",star-len);
	}
return 0;
}
static bool DetermineImageSplitScreenDVD_16_9_Vedio(mm_camera_ch_data_buf_t *frame,QCameraHardwareInterface *mHalCamCtrl)
{//针对16：9视频的分屏判断，

  unsigned int line_count,jj;//第二行开始
  unsigned int shift_count = 0,bottom_shift_count=0;
  unsigned int new_critical_point,new_bottom_critical_point;
  static unsigned int global_count_critical_point = 0,global_count_critical_point_1 = 0;
	if(video_channel_status[0] == '1')//DVD
	{
		 new_critical_point = LookingForABlackCriticalLine(frame,0,300,1);/*向下寻找*/
		 if(new_critical_point)//新的可视数据的临界点
			{
					//	DEBUGLOG("Flyvideo-xx: 顶 临界行在 %d \n",new_critical_point);
					//WriteDataToFrameBuffer(frame,new_critical_point,0,100);
					if(new_critical_point < 10)
					{
							global_count_critical_point_1 = 0;
							new_bottom_critical_point = LookingForABlackCriticalLine(frame,479,70,0);/*向上寻找*/
							if(new_bottom_critical_point < 420)
							{
									global_count_critical_point ++;
									//DEBUGLOG("Flyvideo-xx:global_count_critical_point = %d\n",global_count_critical_point);
									if(global_count_critical_point >15)
									{
											global_count_critical_point = 0;
											if(global_need_rePreview == false )
											{
													global_need_rePreview = true;
													DEBUGLOG("Flyvideo-xx: 16:9 视频边界不对称 分屏");
													if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
														{DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览失败！DVD\n");
														ERR_LOG("Flyvideo Error:CameraRestartPreviewThread faild\n");
														}
													//else
													//	DEBUGLOG("Flyvideo-发现分屏 DVD\n");
											}
									}
							}
					}
					else
					if(new_critical_point != global_last_critical_point)
					{
						global_count_critical_point = 0;
						//DEBUGLOG("Flyvideo-xx: 新的视频临界行%d ！= 旧的临界行 %d\n",new_critical_point,global_last_critical_point);
						shift_count = new_critical_point > global_last_critical_point?new_critical_point - global_last_critical_point:global_last_critical_point - new_critical_point;
						if(shift_count > 8)//偏移2行
						{
							global_count_critical_point_1++;
							//DEBUGLOG("Flyvideo-xx:global_count_critical_point_1 = %d\n",global_count_critical_point_1);
							if(global_count_critical_point_1 >15)
							{
								global_count_critical_point_1 = 0;
								DEBUGLOG("Flyvideo-xx: 顶 偏移 %d\n",shift_count);
								goto FIND_BOTTOM_CRITICAL_POINT;
							}
						}
					}
			}
		 else
			return 0;
	}
return 0;

FIND_BOTTOM_CRITICAL_POINT:
	new_bottom_critical_point = LookingForABlackCriticalLine(frame,479,80,0);/*向上寻找*/
	//if(new_bottom_critical_point)
	goto FIND_BOTTOM_CRITICAL_POINT_2;
return 0;

FIND_BOTTOM_CRITICAL_POINT_2:
	//DEBUGLOG("Flyvideo-xx: 底 临界行在 %d \n",new_bottom_critical_point);
	//WriteDataToFrameBuffer(frame,new_bottom_critical_point,0,100);
bottom_shift_count = new_bottom_critical_point > global_last_bottom_critical_point?new_bottom_critical_point - global_last_bottom_critical_point:global_last_bottom_critical_point - new_bottom_critical_point;
	DEBUGLOG("Flyvideo-xx: 底 偏移 %d\n",bottom_shift_count);
//	if( bottom_shift_count > 8/*底部同时间偏移*/)
	{
			global_last_critical_point = new_critical_point;//更新 新的临界行
			global_last_bottom_critical_point = new_bottom_critical_point;
			if(global_need_rePreview == false )
			{
					global_need_rePreview = true;
					DEBUGLOG("Flyvideo-xx: 16：9视频分屏");
					if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
						{DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览失败！DVD\n");ERR_LOG("Flyvideo Error:CameraRestartPreviewThread faild\n");}
					//else
						//DEBUGLOG("Flyvideo-发现分屏 DVD\n");
			}
	return 1; //确实发生啦分屏；
	}

return 0;
}
static bool DetermineImageSplitScreen(mm_camera_ch_data_buf_t *frame,QCameraHardwareInterface *mHalCamCtrl)
{
	int i =0 ,j =0;
	//static char video_channel_status[10]="1"; //1:DVD 2:AUX 3:Astren
	unsigned char *piont_y;
	unsigned char *piont_crcb;
	unsigned int dete_count = 0;



	if(FlyCameraflymFps<24){DEBUGLOG("Flyvideo-x:FPS = %f",FlyCameraflymFps);return 0;}
	if(++global_fram_at_one_sec_count < 10)//过滤判断次数的频繁度，每隔20帧判断一次，
	{
	return 0;
	}
	else
		global_fram_at_one_sec_count = 0;
	//纵向分屏判断
	if(DetermineImageSplitScreen_Longitudinal(frame,mHalCamCtrl) == 1)
		return 1;

	// property_get("fly.video.channel.status",video_channel_status,"1");//1:DVD 2:AUX 3:Astren
	//以下是横向分屏判断
	/****************************Astren******************************/
	if(video_channel_status[0] == '3')//Astren
	//if(0)
	{//DEBUGLOG("DetermineImageSplitScreen:Astren");
		if(DetermineImageSplitScreen_do_not_or_yes == false)
		{
			return 0;//未发生分屏
		}
		else
		{
			dete_count = 0;
			for(j=0;j<4;j++)
			{
				for(i=0;i<20;i++)
				{
					piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+180*j + i);
					if((*piont_y) < 0x20 || (*piont_y) > BCAR_BLACK_VALUE_UPERR_LIMIT /*0x42*/ )//第一行不全是黑色数据，说明有分屏现象发生
					{
						dete_count++;//一行判断超过10个可疑点才认为是数据异常
							if(dete_count > 5)
							{//DEBUGLOG("Flyvideo-异常数据：buf[%d]= 0x%.2x\n",((180*j) + i),(*piont_y) );
							//DEBUGLOG("DetermineImageSplitScreen:buf[%d]= 0x%.2x\n",((180*j) + i),(*piont_crcb) );
							global_fram_count ++;//数据异常帧数统计
								if(global_fram_count >50)//1sec
								{
									DEBUGLOG("Flyvideo-ERROR1，变量global_need_rePreview 无法达到条件，无法做恢复分屏的动作，现强制去改变这个变量值\n");
									global_need_rePreview = false;//防止CameraRestartPreviewThread阻塞导致，global_need_rePreview无法赋值成true；
									global_fram_count = 0;
									
								}
								if(global_need_rePreview == false && (flynow - Astern_last_time) > ms2ns(2500))//离上次时间超过2.5s
								{
									Astern_last_time = flynow;
									if(global_fram_count >= 1 && RowsOfDataTraversingTheFrameToFindTheBlackLine(frame)) //发现有一帧就重新preview
									{// 第一行有非黑色数据，并且 能够找到黑色行（RowsOfDataTraversingTheFrameToFindTheBlackLine(frame)）
										global_fram_count = 0;
										global_need_rePreview = true;
										//DEBUGLOG("Flyvideo-: need rePreview\n");

										if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
											{DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览失败！Astren\n");ERR_LOG("Flyvideo Error:CameraRestartPreviewThread faild\n");}
										else
											DEBUGLOG("Flyvideo-发现分屏 Astren\n");
										return 1;//返回分屏信息
									}
								}
							goto BREAK_THE;
							}
					}
				}
			}
		}
	}
	/****************************DVD or AUX******************************/
	else
	{
		//DEBUGLOG("DetermineImageSplitScreen:DVD or AUX");
		for(j=0;j<4;j++)
		{
			for(i=0;i<20;i++)
			{//判断第一行的UV值 如何有颜色那么就触发分屏的判断（采用UV做判断的原因是AUX接后坐娱乐输入（CVBS）的顶部有白线）
				piont_crcb = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->cbcr_off+(180*j + i));
				if((*piont_crcb) < (0x7f - 3) || (*piont_crcb) > (0x80+3) )//第一行有颜色说明发生啦分屏,当用户重新调整颜色值后，该处可能有BUG，需要重新调整这两个
				{//DEBUGLOG("Flyvideo-..");
					dete_count++;//一行判断超过10个可疑点才认为是数据异常
						if(dete_count > 9)
						{
						//DEBUGLOG("Flyvideo-异常数据积false累个数达到啦设置值");
						//DEBUGLOG("DetermineImageSplitScreen:buf[%d]= 0x%.2x\n",((180*j) + i),(*piont_crcb) );
						global_fram_count ++;//数据异常帧数统计
							if(global_fram_count >50)//1sec
							{
								DEBUGLOG("Flyvideo-ERROR2，变量global_need_rePreview 无法达到条件，无法做恢复分屏的动作，现强制去改变这个变量值\n");
								global_need_rePreview = false;//防止CameraRestartPreviewThread阻塞导致，global_need_rePreview无法赋值成true；
								global_fram_count = 0;
							}
							if(global_need_rePreview == false)//离上次时间超过10s
							{
								if(global_fram_count >= 1 && RowsOfDataTraversingTheFrameToFindTheBlackLineForDVDorAUX(frame)) //发现有一帧就重新preview ,且寻找到啦黑边
								{
									global_fram_count = 0;
									global_need_rePreview = true;
									//DEBUGLOG("Flyvideo-: need rePreview\n");

									if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
										{DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览失败！DVD or AUX\n");ERR_LOG("Flyvideo Error:CameraRestartPreviewThread faild\n");}
									else
										DEBUGLOG("Flyvideo-发现分屏 DVD or AUX\n");
									return 1;//返回分屏信息
								}
							}
						goto BREAK_THE;
						}
				}

			}

		}
	}
BREAK_THE:
DetermineImageSplitScreenDVD_16_9_Vedio(frame,mHalCamCtrl);
return 0;//未发生分屏
}
static int FlyCameraReadTw9912StatusRegitsterValue()
{//判断这帧数据是否是稳定的，不稳定不显示，针对退出视频时绿色块问题
	unsigned char value,value_1;
	int arg = 0;
	unsigned int cmd;
  if(global_tw9912_file_fd == -1)
	return 0;
	cmd = COPY_TW9912_STATUS_REGISTER_0X01_4USER;
	if (ioctl(global_tw9912_file_fd,cmd, &arg) < 0)
        {
		DEBUGLOG("Flyvideo-: Call cmd COPY_TW9912_STATUS_REGISTER_0X01_4USER fail\n");
		ERR_LOG("Flyvideo Error:Call cmd COPY_TW9912_STATUS_REGISTER_0X01_4USER fail\n");
		return 0;
	}
	read(global_tw9912_file_fd, (void *)(&value),sizeof(unsigned char));
	//DEBUGLOG("Flyvideo-:0x%.2x\n",value);

	value_1 = value & 0x68;
	if(value_1 != 0x68)
	{
		//global_fream_give_up ++ ;
		if(global_fream_give_up < 10)
		{
		DEBUGLOG("Flyvideo-:Vedio singnal bad\n");
		ERR_LOG("Flyvideo :Vedio singnal bad\n");
		//memset((void *)(frame->def.frame->buffer+frame->def.frame->y_off),0,720*480);
		//memset((void *)(frame->def.frame->buffer+frame->def.frame->cbcr_off),0,720*480);
		return 1;
		}
		else
		{// hope 10 fream ago signal is good 
		global_fream_give_up = 0;
		return 0;
		}
	}
return 0;
}
static void FlyCameraAuxBlackLine(void)
{//写黑色数据到 帧缓存中，可以屏蔽这个动作看视频显示表现就知道区别
	if(video_channel_status[0] == '2')//AUX
			{
				memset((void *)(frame->def.frame->buffer+frame->def.frame->y_off),0x1e,720*3);//黑前三行
				memset((void *)(frame->def.frame->buffer+frame->def.frame->cbcr_off),0x7f,720*2);

				if(video_format[0] == '4' || video_format[0] == '2')
				;//pal
				else
				{
				memset((void *)(frame->def.frame->buffer+frame->def.frame->cbcr_off+720*237),0x80,720*4);//黑后3行
				memset((void *)(frame->def.frame->buffer+frame->def.frame->y_off+720*475),0x1e,720*5);
				}
			}
}
bool FlyCameraFrameDisplayOrOutDisplay()
{
bool ret;
		if(video_channel_status[0] != '1')//is not DVD
		{//判断视频是否稳定，不稳定这帧不显示，改函数用来改善倒车推车有大块绿条问题
			ret = FlyCameraReadTw9912StatusRegitsterValue();//1:DVD 2:AUX 3:Astren
			if(ret == 1) return 1;
		}


		if(video_channel_status[0] != '3')
		{//倒车状态先判断黑屏再判断分屏
			if( DetermineImageSplitScreen(frame,mHalCamCtrl) )//发现分屏这个帧丢弃不显示
			{
				FlyCameraAuxBlackLine();			
				return 1;
			}
			else 
				FlyCameraAuxBlackLine();
		}
		else//Astren
		{
			//if(BlackJudge(frame) == 1)//发现黑屏
				//return processPreviewFrameWithOutDisplay(frame);
			//	DEBUGLOG("Flyvideo-：发现黑屏，但是目前还未作任何处理\n");
			//else
			//	{
				if( DetermineImageSplitScreen(frame,mHalCamCtrl) )//发现分屏
					return 1;
				else
					return 0;
			//	}
			//return 0;
		}
return 0;
}
}
