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
#if 1
#define DEBUGLOG LOGE
#else
#define DEBUGLOG do{}while(0)
#endif

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

	static bool ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = false;
	static char video_format[10]="1";
	static char video_show_status[10]="0";
	static mm_camera_ch_data_buf_t *frame;
	static QCameraHardwareInterface *mHalCamCtrl;
	static unsigned int open_dev_fail_count=0;
	static unsigned int global_fram_at_one_sec_count = 0;
	static unsigned int Longitudinal_last_fream_count =9;
	static SplitScreenInfo LongitudinalInformationRemember={300,0,0,true,0};
	static bool global_need_rePreview = false;
	static pthread_t thread_DetermineImageSplitScreenID = NULL;
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
  static int global_tw9912_file_fd;
  static int global_fream_give_up =0;
void FlyCameraStar()
{
	globao_mFlyPreviewStatus = 1;
	global_fream_give_up = 0;
	DEBUGLOG("Flyvideo-mFlyPreviewStatus =%d\n",globao_mFlyPreviewStatus);
	global_tw9912_file_fd = open("/dev/tw9912config",O_RDWR);
	if(global_tw9912_file_fd ==-1)
		{
		DEBUGLOG("Flyvideo-:Error FlyCameraStar() tw9912config faild\n");
		}
}
void FlyCameraStop()
{
rePreview_count = 0;
ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = false; //at next preview ,again run function
open_dev_fail_count = 0;
global_fram_at_one_sec_count = 0;
LongitudinalInformationRemember.ThisIsFirstFind = true;
LongitudinalInformationRemember.BegingFindBlackFreamCount = 0;
DetermineImageSplitScreen_do_not_or_yes = true;//at next preview ,allow run function DetermineImageSplitScreen at astren
globao_mFlyPreviewStatus = 0;
global_Fream_is_first = true;
close(global_tw9912_file_fd);
DEBUGLOG("Flyvideo-mFlyPreviewStatus =%d\n",globao_mFlyPreviewStatus);
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
bool FlyCameraImageDownFindBlackLine()
{
		if(video_channel_status[0] == '3' && ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok == false)//onle at Astren and not PAL
			{
				if(ToFindBlackLineAndSetTheTw9912VerticalDelayRegister(frame) == 1)
					{
					DEBUGLOG("Flyvideo-注意：图像静帧一帧");
					return 1;
					}
			}
return 0;
}
void FlyCameraThisIsFirstOpenAtDVD()
{
	if(video_channel_status[0] == '1' && global_Fream_is_first == true)
			{
						 int file_fd;
						 TW9912Info tw9912_info;
						 file_fd = open("/dev/tw9912config",O_RDWR);
						 if(file_fd ==-1)
						 {
							       DEBUGLOG("Flyvideo-错误，打开设备节点文件“/dev/tw9912config”失败");
							      // return -1;
											goto OPEN_ERR;
						 }
						 read(file_fd, (void *)(&tw9912_info),sizeof(TW9912Info));
						 if(tw9912_info.this_is_first_open == true)
						 {
							       DEBUGLOG("Flyvideo-x:第一次打开DVD,重新preview\n");
							       mHalCamCtrl->stopPreview();
							       //      sleep(1);
							       mHalCamCtrl->startPreview();
							       //      sleep(1);
						 }
						 tw9912_info.this_is_first_open = false;
						 write(file_fd, (const void *)(&tw9912_info),sizeof(TW9912Info));
						 close(file_fd);
							OPEN_ERR:
						 global_Fream_is_first = false;
			}
}
void FlyCameraNotSignalAtLastTime()
{
	if(video_show_status[0] == '0' && rePreview_count > 100)
		{//发现黑屏 且 视频在上次打开没有视频源输入
			DEBUGLOG("Flyvideo-上次视频打开，视频源无视频输入，stopPreview()-->\n");
			mHalCamCtrl->stopPreview();
			DEBUGLOG("Flyvideo-上次视频打开，视频源无视频输入，startPreview()-->\n");
			mHalCamCtrl->startPreview();
			rePreview_count = 0;
		}
		else if(video_show_status[0] == '0')
		{
			rePreview_count++;
			if(rePreview_count >10000)rePreview_count = 101;
		}
}
void *CameraRestartPreviewThread(void *mHalCamCtrl1)
{
	QCameraHardwareInterface *mHalCamCtrl = (QCameraHardwareInterface *)mHalCamCtrl1;
	usleep(1000*500);//500 ms
	if(globao_mFlyPreviewStatus == 1)
	{
		DEBUGLOG("Flyvideo-DetermineImageSplitScreen:stopPreview()-->\n");
		mHalCamCtrl->stopPreview();
		//	sleep(1);

		DEBUGLOG("Flyvideo-DetermineImageSplitScreen:startPreview()-->\n");
		mHalCamCtrl->startPreview();
		//	sleep(1);
	}
	else
	{
	DEBUGLOG("Flyvideo-发现分屏需要重新preview，但是视频HAL调用啦stop。\n");
	}

	LongitudinalInformationRemember.ThisIsFirstFind = true;
	global_need_rePreview = false;
    return NULL;
}

static bool ToFindBlackLine(mm_camera_ch_data_buf_t *frame)
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
				if((*piont_y) >= 0x20 && (*piont_y) <= 0x35 )
				{
					black_count++;
					if( (719-i+black_count)< 700)//一定无法达到700个点的要求，没必要再执行下去。
					return 0;
					if( black_count > 700)//“黑色"数据大于700认为是黑线出现
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
	int file_fd;
	TW9912Info tw9912_info;
	file_fd = open("/dev/tw9912config",O_RDWR);
		if(file_fd ==-1)
		{
				if(open_dev_fail_count == 0)
				{
						DEBUGLOG("Flyvideo-错误x，打开设备节点文件“/dev/tw9912config”失败");
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
						if(file_fd ==-1)
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

	read(file_fd, (void *)(&tw9912_info),sizeof(TW9912Info));
	if(tw9912_info.flag == true)
	{
		if(ToFindBlackLine(frame) ==1)//找到啦黑色数据行
		{
			tw9912_info.sta = true;
			tw9912_info.flag = false;
			write(file_fd, (const void *)(&tw9912_info),sizeof(TW9912Info));
		DEBUGLOG("Flyvideo-找到了黑色数据行，图像下移策略不需再执行");
		ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = true;
		DetermineImageSplitScreen_do_not_or_yes = true;
		}
		else//还没找到黑色数据行 图像下移一行
		{
			if(tw9912_info.reg_val > 0x10 && tw9912_info.reg_val <= 0x17)
			{
				tw9912_info.reg = 0x08;
				tw9912_info.reg_val -= 1;
				write(file_fd, (const void *)(&tw9912_info),sizeof(TW9912Info));
				DEBUGLOG("Flyvideo-黑色数据行还未找到，图像下移一个像素点");
			}
			else
			{
				DEBUGLOG("Flyvideo-警告：图像顶部腾出的黑线还未找到，但是图像下移数据的设置已经达到下限，不能再设置更小");
				ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = true;
				DEBUGLOG("Flyvideo-警告：重要！！由于腾出黑线的操作无法完成，分屏判断将失效");
				DetermineImageSplitScreen_do_not_or_yes = false;
			}
		return 1;
		}

	}
	else
	{
		DEBUGLOG("Flyvideo-从内核读回的数据 tw9912_info.flag == false 无需再“腾”顶部的黑线");
		ToFindBlackLineAndSetTheTw9912VerticalDelayRegister_is_ok = true;
	}
	close(file_fd);
OPEN_ERR:
return 0;
}
static bool VideoItselfBlackJudge(mm_camera_ch_data_buf_t *frame)
{
	unsigned int black_count;
	int i =0 ,j =0;
	unsigned char *piont_y;
	for(;j<480;j++)
		{i++;
			piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*j + i);//取值 正方形 斜线查找
			if(*piont_y <= 0x20)//找到啦一个黑色点
				black_count ++;

			if( (480 - j + black_count) < 450)//一定无法达到470个点的要求，没必要再执行下去。
						continue;

			if(black_count>450)
				{
					DEBUGLOG("Flyvideo-:黑");
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
	int i = 0,jj =0;
	unsigned int count;
	unsigned int find_line = 477;//NTSC
	if(video_format[0] == '4' || video_format[0] == '2')
	{
	DEBUGLOG("Flyvideo-:视频制式是:PAL，寻找范围是：9～562");
	find_line = 562;//PAL
	}
	else DEBUGLOG("Flyvideo-:视频制式是:NTSC，寻找范围是：9～477");
			DEBUGLOG("Flyvideo-可能发生分屏，寻找黑边中。。。");
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
return 0;
Break_The_Func:
DEBUGLOG("Flyvideo-：确定发生啦分屏,黑边位置在第 %d 行，发现的黑色数据个数= %d 个,其中一个黑色数据是0x%.2x",i,count,*piont_y);
return 1;//确定这帧是出现啦分屏
}
static bool RowsOfDataTraversingTheFrameToFindTheBlackLine(mm_camera_ch_data_buf_t *frame)
{
	unsigned char *piont_y;
	int i = 0,jj =0;
	unsigned int count;
	unsigned int find_line = 477;//NTSC
	if(video_format[0] == '4' || video_format[0] == '2')
	{
	DEBUGLOG("Flyvideo-:视频制式是:PAL，寻找范围是：9～562");
	find_line = 562;//PAL
	}
	else DEBUGLOG("Flyvideo-:视频制式是:NTSC，寻找范围是：9～477");
			DEBUGLOG("Flyvideo-可能发生分屏，寻找黑边中。。。");
			for(i=9;i<find_line;i++)//某列下的第几行，找 一个点 看数据是否是黑色；前10行和后10行放弃找，正常情况下前3行是黑色的数据
			{
				piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + 300);//在第300列下的每行找黑点
				//if(*piont_y <= 0x20)//到此，在某列下的某行，找到啦一个黑点，接下来对这一行，遍历700个点，看这行是否确实都是黑色数据
				if((*piont_y) >= 0x20 && (*piont_y) <= 0x35 )
				{
					for(jj=0;jj<719;jj++)//在一行的遍历
					{
						piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + jj);
						//if(*piont_y <= 0x20)
						if((*piont_y) >= 0x20 && (*piont_y) <= 0x35 )
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
return 0;
Break_The_Func:
DEBUGLOG("Flyvideo-：确定发生啦分屏,黑边位置在第 %d 行，发现的黑色数据个数= %d 个,其中一个黑色数据是0x%.2x",i,count,*piont_y);
return 1;//确定这帧是出现啦分屏
}
//Determine whether the split-screen
static bool DetermineImageSplitScreen_Longitudinal(mm_camera_ch_data_buf_t *frame,QCameraHardwareInterface *mHalCamCtrl)
{
	int i =0 ,j = 30,j_end = 690;
	unsigned char *piont_y;
	unsigned int dete_count = 0;
	Longitudinal_last_fream_count ++;
	if(Longitudinal_last_fream_count > 0xfffe) Longitudinal_last_fream_count = 9;
	if( Longitudinal_last_fream_count < 10)//时间过滤，防止短时间内，连续的出现preview操作
	{//DEBUGLOG("Flyvideo-:T");
			return 0;
	}
	if(LongitudinalInformationRemember.ThisIsFirstFind == false)
	{//连续5帧还没做完判断，重新定位黑边坐标
		LongitudinalInformationRemember.BegingFindBlackFreamCount++;
		if(LongitudinalInformationRemember.BegingFindBlackFreamCount > 6 || (6 - LongitudinalInformationRemember.BegingFindBlackFreamCount +LongitudinalInformationRemember.BlackFreamCount ) < 5 )//已经超过统计数，或者 剩下的加已经统计的少于10个
		{DEBUGLOG("Flyvideo-:纵向分屏累计清零");
			LongitudinalInformationRemember.BegingFindBlackFreamCount = 0;
			LongitudinalInformationRemember.ThisIsFirstFind = true;
		}
	}
	if(LongitudinalInformationRemember.ThisIsFirstFind == false)//如果已经定位啦黑色边,不再做横向遍历查找黑点坐标
	{
		j = LongitudinalInformationRemember.BlackCoordinateX;
		j_end = j;
	}
	for(;j<=j_end;j++)
		{
			piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*300 + j);//在第300行，横向扫描
			if(*piont_y <= 0x20)//找到啦一个黑色点
			{
				for(i=10;i<470;i++)
				{
					piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+720*i + j);
					if((*piont_y) <= 0x20)
					{
						dete_count++;
						if( (460 - (i-10) + dete_count) < 455)//一定无法达到400个点的要求，没必要再执行下去。
							{
								dete_count = 0;
								goto THE_FOR;//跳到下一个黑点对应的列统计。
							}
						if(dete_count > 454 && VideoItselfBlackJudge(frame) == 1)//提前一帧判断是否是黑色数据帧
							return 0;//未发生分屏
						if(dete_count > 455)//在这个纵向行黑色点个数达到啦设置值,且这帧本身不是黑色屏
						{
							dete_count = 0;
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
									Longitudinal_last_fream_count = 0;//时间标签更新
									global_need_rePreview = true;
									LongitudinalInformationRemember.BlackFreamCount = 0;
									#if 0
											DEBUGLOG("Flyvideo-:纵向分屏,stopPreview()-->\n");
											mHalCamCtrl->stopPreview();
											DEBUGLOG("Flyvideo-:纵向分屏,startPreview()-->\n");
											mHalCamCtrl->startPreview();
									#else
											if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
												DEBUGLOG("Flyvideo-:纵向分屏，创建线程重新预览失败！\n");
											else
												DEBUGLOG("Flyvideo-:纵向分屏，创建线程重新预览成功！\n");
									#endif
									global_need_rePreview = false;
									LongitudinalInformationRemember.ThisIsFirstFind = true;
									LongitudinalInformationRemember.BegingFindBlackFreamCount = 0;
									return 1;//返回分屏信息
								}
							}
						goto BREAK_THE;
						}
					}

				}
				THE_FOR:
				;
			}
		}
BREAK_THE:
return 0;//未发生分屏
}
static bool DetermineImageSplitScreen(mm_camera_ch_data_buf_t *frame,QCameraHardwareInterface *mHalCamCtrl)
{
	int i =0 ,j =0;
	//static char video_channel_status[10]="1"; //1:DVD 2:AUX 3:Astren
	unsigned char *piont_y;
	unsigned char *piont_crcb;
	unsigned int dete_count = 0;



	if(FlyCameraflymFps<24){DEBUGLOG("Flyvideo-x:flymFps = %f",FlyCameraflymFps);return 0;}
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
			for(j=0;j<4;j++)
			{
				for(i=0;i<20;i++)
				{
					piont_y = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->y_off+180*j + i);
					if((*piont_y) < 0x20 || (*piont_y) > 0x35 )
					{
						dete_count++;//一行判断超过10个可疑点才认为是数据异常
							if(dete_count > 5)
							{//DEBUGLOG("Flyvideo-异常数据：buf[%d]= 0x%.2x\n",((180*j) + i),(*piont_y) );
							//DEBUGLOG("DetermineImageSplitScreen:buf[%d]= 0x%.2x\n",((180*j) + i),(*piont_crcb) );
							global_fram_count ++;//数据异常帧数统计
								if(global_fram_count >50)//1sec
								{
									global_need_rePreview = false;//防止CameraRestartPreviewThread阻塞导致，global_need_rePreview无法赋值成true；
									global_fram_count = 0;
								}
								if(global_need_rePreview == false && (flynow - Astern_last_time) > ms2ns(2500))//离上次时间超过2.5s
								{
									Astern_last_time = flynow;
									if(global_fram_count >= 1 && RowsOfDataTraversingTheFrameToFindTheBlackLine(frame)) //发现有一帧就重新preview
									{
										global_fram_count = 0;
										global_need_rePreview = true;
										//DEBUGLOG("Flyvideo-: need rePreview\n");

										if( pthread_create(&thread_DetermineImageSplitScreenID, NULL,CameraRestartPreviewThread, (void *)mHalCamCtrl) != 0)
											DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览失败！Astren\n");
										else
											DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览成功！Astren\n");
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
			{
				piont_crcb = (unsigned char *)(frame->def.frame->buffer+frame->def.frame->cbcr_off+(180*j + i));
				if((*piont_crcb) < (0x7f - 3) || (*piont_crcb) > (0x80+3) )//第一行有颜色说明发生啦分屏
				{//DEBUGLOG("Flyvideo-..");
					dete_count++;//一行判断超过10个可疑点才认为是数据异常
						if(dete_count > 9)
						{
						DEBUGLOG("Flyvideo-异常数据积累个数达到啦设置值");
						//DEBUGLOG("DetermineImageSplitScreen:buf[%d]= 0x%.2x\n",((180*j) + i),(*piont_crcb) );
						global_fram_count ++;//数据异常帧数统计
							if(global_fram_count >50)//1sec
							{
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
										DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览失败！DVD or AUX\n");
									else
										DEBUGLOG("Flyvideo-发现分屏，创建线程重新预览成功！DVD or AUX\n");
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

return 0;//未发生分屏
}
static int FlyCameraReadTw9912StatusRegitsterValue()
{
	int file_fd;
	unsigned char value,value_1;
	int arg = 0;
	unsigned int cmd;
       if(global_tw9912_file_fd == -1)
	return 0;
	cmd = COPY_TW9912_STATUS_REGISTER_0X01_4USER;
	if (ioctl(global_tw9912_file_fd,cmd, &arg) < 0)
        {
		DEBUGLOG("Flyvideo-: Call cmd COPY_TW9912_STATUS_REGISTER_0X01_4USER fail\n");
		close(file_fd);
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
{
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
/*bool ret;
		if(video_channel_status[0] != '1')//is not DVD
		{
			ret = FlyCameraReadTw9912StatusRegitsterValue();//1:DVD 2:AUX 3:Astren
			if(ret == 1) return 1;
		}
*/

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
