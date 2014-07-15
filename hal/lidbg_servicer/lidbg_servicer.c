
#include "lidbg_servicer.h"

#define LOG_COUNT_MAX (16)

#define SERVICER_DONOTHING  (0)
#define LOG_DMESG  (1)
#define LOG_LOGCAT (2)
#define LOG_ALL (3)
#define LOG_CONT    (4)
#define LOG_CLEAR_LOGCAT_KMSG (5)
#define LOG_SHELL_TOP_DF_PS (6)

#define WAKEUP_KERNEL (10)
#define SUSPEND_KERNEL (11)

#define LOG_DVD_RESET (64)
#define LOG_CAP_TS_GT811 (65)
#define LOG_CAP_TS_FT5X06 (66)
#define LOG_CAP_TS_FT5X06_SKU7 (67)
#define LOG_CAP_TS_RMI (68)
#define LOG_CAP_TS_GT801 (69)
#define CMD_FAST_POWER_OFF (70)
#define LOG_CAP_TS_GT911 (71)
#define LOG_CAP_TS_GT910 (72)
#define UMOUNT_USB (80)
#define VIDEO_SET_PAL (81)
#define VIDEO_SET_NTSC (82)

#define CMD_FLY_POWER_OFF (101)


#define VIDEO_SHOW_BLACK (85)
#define VIDEO_NORMAL_SHOW (86)

#define UBLOX_EXIST   (88)
#define CMD_ACC_OFF_PROPERTY_SET (89)

#define VIDEO_PASSAGE_AUX (90)
#define VIDEO_PASSAGE_ASTERN (91)
#define VIDEO_PASSAGE_DVD (92)

#define CMD_ACC_OFF (93)
#define CMD_ACC_ON (94)
#define CMD_IGNORE_WAKELOCK (95)

pthread_t ntid;
int fd = 0;
static int ts_nod_fd, ret;
#include "./copyfile.c"
#ifdef SHARE_MMAP_ENABLE
#include "./mmap.c"
#endif

struct lidbg_dev_smem *plidbg_smem = NULL;

static char logcat_name[64];
static char kmsg_name[64];
int read_from_file(const char *path, char *buf, size_t size)
{
    if (!path)
        return -1;
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1)
    {
        lidbg("Could not open '%s'", path);
        return -1;
    }
    ssize_t count = read(fd, buf, size);
    if (count > 0)
    {
        while (count > 0 && buf[count - 1] == '\n')
            count--;
        buf[count] = '\0';
    }
    else
    {
        buf[0] = '\0';
    }
    close(fd);
    return count;
}
void get_time(char *datastring)
{
    struct tm *local;
    const char *tmFormat = "%Y%m%d_%I%M%S";
    time_t now = time(NULL);
    local = localtime(&now);
    if(datastring)
        strftime(datastring, 64, tmFormat, local);
}
void compose_log_name(char *logname, unsigned int log_type)  //log_type 0: logcat     1:kmsg
{
    char mchine_id[32];
    char current_time[32];

    get_time(current_time);
    if ( read_from_file("/data/lidbg/MIF.txt", mchine_id, sizeof(mchine_id)) > 0)
    {
        if(log_type == 0)
            sprintf(logname, "logcat_%s_%s.txt", mchine_id, current_time);
        else
            sprintf(logname, "kmsg_%s_%s.txt", mchine_id, current_time);
    }
    else
    {
        if(log_type == 0)
            sprintf(logname, "logcat_default_%s.txt", current_time);
        else
            sprintf(logname, "kmsg_default_%s.txt", current_time);
    }
    lidbg("compose_log_name:new.[%s]\n", logname);
}

void thread_fastboot(void)
{
    lidbg("thread_fastboot+\n");
    system("am broadcast -a android.intent.action.FAST_BOOT_START");
    lidbg("thread_fastboot-\n");
    pthread_exit(0);

}


void lunch_fastboot()
{
    pthread_t id1;
    int ret;
    ret = pthread_create(&id1, NULL, (void *)thread_fastboot, NULL);
    if(ret != 0)
    {
        lidbg("Create pthread error!\n");
    }
}

int  servicer_handler(int signum)
{

    int cmd = 0;
    static int count = 0;
    int readlen;
    lidbg("servicer_handler++\n");

loop_read:

    readlen = read(fd, &cmd, 4);

    if(cmd == SERVICER_DONOTHING)
    {
        lidbg("servicer_handler-\n");
        return SERVICER_DONOTHING;
    }
    else
    {
        lidbg("cmd = %d\n", cmd);

        switch(cmd)
        {
        case CMD_ACC_OFF :
        {
            lidbg("CMD_ACC_OFF+++\n");
            system("am broadcast -a cn.flyaudio.boot.ACCOFF &");
            lidbg("CMD_ACC_OFF---\n");
            break;
        }
        case CMD_ACC_ON :
        {
            lidbg("CMD_ACC_ON+++\n");
            system("am broadcast -a cn.flyaudio.boot.ACCON &");
            lidbg("CMD_ACC_ON---\n");
            break;
        }
        case CMD_FAST_POWER_OFF :
        {
            lidbg("CMD_FAST_POWER_OFF+++\n");
            lunch_fastboot();
            lidbg("CMD_FAST_POWER_OFF---\n");
            break;
        }


        case CMD_FLY_POWER_OFF:
        {
            lidbg("CMD_FLY_POWER_OFF+++\n");
            system("am broadcast -a cn.flyaudio.intent.action.FAST_BOOT_START &");
            lidbg("CMD_FLY_POWER_OFF---\n");
            break;
        }
        case CMD_IGNORE_WAKELOCK:
        {
            lidbg("CMD_IGNORE_WAKELOCK+++\n");
            system("am broadcast -a com.android.FORCESUSPEND &");
            lidbg("CMD_IGNORE_WAKELOCK---\n");
            break;
        }

        case LOG_LOGCAT :
        {
            static int flag = 0;
            char cmd[128];
            if(flag)break;
            else flag = 1;
            lidbg("logcat+\n\n");

            compose_log_name(logcat_name, 0);

            sprintf(cmd, "date >/data/%s", logcat_name);
            system(cmd);
            memset(cmd, '\0', sizeof(cmd));
            sleep(1);
            system("chmod 777 /data/logcat*");
            sleep(1);
            sprintf(cmd, "logcat  -v time>> /data/%s &", logcat_name);
            system(cmd);
            lidbg("logcat-\n");
            break;
        }
        case LOG_DMESG :
        {
            static int flag = 0;
            char cmd[128];
            if(flag)break;
            else flag = 1;
            lidbg("\n\n\nkmsg+\n\n");

            system("echo c lidbg_trace_msg disable > /dev/mlidbg0");
            compose_log_name(kmsg_name, 1);

            sprintf(cmd, "date >/data/%s", kmsg_name);
            system(cmd);
            memset(cmd, '\0', sizeof(cmd));
            sleep(1);
            system("chmod 777 /data/kmsg*");
            sleep(1);
            sprintf(cmd, "cat /proc/kmsg >> /data/%s &", kmsg_name);
            system(cmd);
            lidbg("kmsg-\n");
            break;
        }
        case LOG_CLEAR_LOGCAT_KMSG :
        {
            lidbg("clear+logcat*&&kmsg*\n");
            system("rm /data/logcat*");
            system("rm /data/kmsg*");
            lidbg("clear-logcat*&&kmsg*\n");
            break;
        }
        case LOG_SHELL_TOP_DF_PS :
        {
            lidbg("\n\n\nLOG_SHELL_TOP_DF_PS+\n\n");
            system("date > /data/machine.txt");
            system("cat /proc/cmdline >> /data/machine.txt");
            system("getprop fly.version.mcu >> /data/machine.txt");
            system("top -n 3 -t >/data/top.txt &");
            system("screencap -p /data/screenshot.png &");
            system("ps > /data/ps.txt");
            system("df > /data/df.txt");
            system("chmod 777 /data/*.txt");						
            system("chmod 777 /data/*.png");						
            lidbg("\n\n\nLOG_SHELL_TOP_DF_PS-\n\n");
            break;
        }

        case VIDEO_SET_PAL:
        {
            lidbg("<<<<< now get QCamera set pal\n");
            property_set("tcc.fly.vin.pal", "1");// 1 is pal 0 is ntsc
            break;

        }
        case VIDEO_SET_NTSC:
        {
            lidbg("<<<<< now get QCamera set ntsc\n");
            property_set("tcc.fly.vin.pal", "0");// 1 is pal 0 is ntsc
            break;

        }

        case VIDEO_SHOW_BLACK:
        {
            lidbg("<<<<< now Set Video show black.\n");
            property_set("fly.video.show.status", "0");// 0 is black
            break;

        }
        case VIDEO_NORMAL_SHOW:
        {
            lidbg("<<<<< now Set Video normal show.\n");
            property_set("fly.video.show.status", "1");// 1 is normal
            break;

        }
        case VIDEO_PASSAGE_DVD:
        {
            lidbg("<<<<< now Set Video channel is DVD.\n");
            property_set("fly.video.channel.status", "1");// 1 is DVD 2 is AUX 3 is Astern
            break;

        }
        case VIDEO_PASSAGE_AUX:
        {
            lidbg("<<<<< now Set Video channel is aux.\n");
            property_set("fly.video.channel.status", "2");// 1 is DVD 2 is AUX 3 is Astern
            break;

        }
        case VIDEO_PASSAGE_ASTERN:
        {
            lidbg("<<<<< now Set Video channel is Astren.\n");
            property_set("fly.video.channel.status", "3");// 1 is DVD 2 is AUX 3 is Astern
            break;
        }
        }
    }
    goto loop_read;

}



int main(int argc , char **argv)
{
    int err = 0;
    int cmd = 0;
    int count = 0;
    int oflags;

    lidbg("lidbg_servicer start\n");


#if (defined(BOARD_V1) || defined(BOARD_V2))
    system("echo 600000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
    system("echo 600000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq");
#endif

open_dev:
    fd = open("/dev/mlidbg0", O_RDWR);
    if((fd == 0xfffffffe) || (fd == 0) || (fd == 0xffffffff))
    {
        lidbg("open mlidbg0 fail\n");
        sleep(1);//delay wait for /dev/lidbg_servicer to creat
        goto open_dev;
    }
    close(fd);


    lidbg("open mlidbg0 ok\n");

    sleep(1);
    system("chmod 0777 /dev/mlidbg0");
    system("chmod 0777 /dev/lidbg_servicer");
    system("chmod 0777 /dev/lidbg_msg");
    system("chmod 0777 /dev/ubloxgps0");

    fd = open("/dev/lidbg_servicer", O_RDWR);
    if((fd == 0xfffffffe) || (fd == 0) || (fd == 0xffffffff))
    {
        lidbg("open lidbg_servicer fail\n");
        goto open_dev;

    }

    signal(SIGIO, servicer_handler); //?input_handler()??SIGIO??
    fcntl(fd, F_SETOWN, getpid());
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags | FASYNC); //?????fasync_helper


    //clear fifo
    while(1)
    {
        if(servicer_handler(0) == SERVICER_DONOTHING)
            break;
    }

#if 1
    //chegnweidong
    system("insmod /flysystem/lib/mdrv/flysemdriver.ko");
    system("insmod /flysystem/lib/tcdriver/uuid.ko");
    //for flycar
    sleep(1);
    system("insmod /flysystem/lib/modules/FlyDebug.ko");
    system("insmod /flysystem/lib/modules/FlyMmap.ko");
    system("insmod /flysystem/lib/modules/FlyHardware.ko");
    system("insmod /flysystem/lib/modules/FlyHardwareDevice.ko");
    system("insmod /flysystem/lib/modules/FlyAudio.ko");
    system("insmod /flysystem/lib/modules/FlyAudioDevice.ko");
    system("insmod /flysystem/lib/modules/productinfo.ko");
    system("insmod /flysystem/lib/modules/vendor_flyaudio.ko");
    system("insmod /flysystem/lib/modules/FlyDR.ko");
    system("insmod /flysystem/lib/modules/FlyAS.ko");
    system("insmod /flysystem/lib/modules/FlyReturn.ko");

    sleep(1);
    system("chmod 0777 /dev/ubloxgps0");
    system("chmod 0777 /dev/FlyDebug");
    system("chmod 0777 /dev/FlyMmap ");
    system("chmod 0777 /dev/FlyHardware");
    system("chmod 0777 /dev/FlyAudio");

    system("chmod 0777 /dev/FlyDR");
    system("chmod 0777 /dev/FlyAS");
    system("chmod 0777 /dev/FlyReturn");


    //chegnweidong
    system("chmod 0777 /dev/flysemdriver");
    system("chmod 606 /dev/tw9912config");

    sleep(5);
    system("chmod 0777 /dev/ubloxgps0");

    system("chmod 777 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    system("chmod 777 /sys/power/state");
    system("chmod 777 /proc/fake_suspend");
    system("chmod 777 /proc/fake_wakeup");
    system("chmod 777 /mnt/state.txt");

    system("chmod 0666 /dev/mtd/mtd1");
#endif

#ifdef SHARE_MMAP_ENABLE
    plidbg_smem = (struct lidbg_dev_smem *)get_mmap();
    //test ok
    lidbg("read smem:%x,%d\n", plidbg_smem->smemaddr, plidbg_smem->smemsize);
#endif

    sleep(30);

#if (defined(BOARD_V1) || defined(BOARD_V2))
    ///////low mem kill
    if(1)
    {

        system("chmod 777 /sys/module/lowmemorykiller/parameters/minfree");
        // cat /sys/module/lowmemorykiller/parameters/minfree
        //system("echo 3674,4969,6264,8312,9607,11444 > /sys/module/lowmemorykiller/parameters/minfree");//origin
        sleep(1);
        lidbg("set minfree\n");
        system("echo 3674,4969,6264,6264,6264,6264 > /sys/module/lowmemorykiller/parameters/minfree");
    }

    ////////set cpu fre
#endif
    //cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq

    DUMP_BUILD_TIME;
    DUMP_BUILD_TIME_FILE;

#if (defined(BOARD_V1) || defined(BOARD_V2))
    lidbg("BOARD_V2\n");
#elif defined(BOARD_V3)
    lidbg("BOARD_V3\n");
#endif

    if(access("/data/core.txt", R_OK) == 0)//NOTE: tmp.clear the history.del it later
        system("rm /data/*.txt");

    while(1)
    {
        sleep(60);
        //lidbg(".\n");
        //system("chmod 777 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        //system("echo 1008000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
        //system("echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        //echo "userspace" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        //echo 600000 > /sys/devices/system/cpu/cpufreq/ondemand/scaling_setspeed
        //cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq
        //sleep(10);
    }
    return 0;
}
