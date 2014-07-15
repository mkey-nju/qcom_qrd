

/*本程序符合GPL条约
MyCom.c
*/


#include <stdio.h>              // printf
#include <fcntl.h>              // open
#include <string.h>             // bzero
#include <stdlib.h>             // exit
#include <sys/times.h>          // times
#include <sys/types.h>          // pid_t
#include <termios.h>            //termios, tcgetattr(), tcsetattr()
#include <unistd.h>
#include <sys/ioctl.h>          // ioctl
#include "serial_test.h"



#define TTY_DEV "/dev/tcc-uart" //端口路径   

#define TIMEOUT_SEC(buflen,baud) (buflen*20/baud+2)  //接收超时   
#define TIMEOUT_USEC 0
/*******************************************
 *  获得端口名称
********************************************/
char *get_ptty(pportinfo_t pportinfo)
{
    char *ptty = NULL;

    switch(pportinfo->tty)
    {
    case '0':
    {
        ptty = TTY_DEV"0";
    }
    break;
    case '1':
    {
        ptty = TTY_DEV"1";
    }
    break;
    case '2':
    {
        ptty = TTY_DEV"2";
    }
    break;

    case '3':
    {
        ptty = TTY_DEV"3";
    }
    break;

    case '4':
    {
        ptty = TTY_DEV"4";
    }
    break;

    case '5':
    {
        ptty = TTY_DEV"5";
    }
    break;

    case '6':
    {
        ptty = TTY_DEV"6";
    }
    break;
    }

    return(ptty);
}

/*******************************************
 *  波特率转换函数（请确认是否正确）
********************************************/
int convbaud(unsigned long int baudrate)
{
    switch(baudrate)
    {
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    default:
        return B9600;
    }
}

/*******************************************
 *  Setup comm attr
 *  fdcom: 串口文件描述符，pportinfo: 待设置的端口信息（请确认）
 *
********************************************/
int PortSet(int fdcom, const pportinfo_t pportinfo)
{
    struct termios termios_old, termios_new;
    int     baudrate, tmp;
    char    databit, stopbit, parity, fctl;

    bzero(&termios_old, sizeof(termios_old));
    bzero(&termios_new, sizeof(termios_new));
    cfmakeraw(&termios_new);
    tcgetattr(fdcom, &termios_old);         //get the serial port attributions
    /*------------设置端口属性----------------*/
    //baudrates
    baudrate = convbaud(pportinfo -> baudrate);
    cfsetispeed(&termios_new, baudrate);        //填入串口输入端的波特率
    cfsetospeed(&termios_new, baudrate);        //填入串口输出端的波特率
    termios_new.c_cflag |= CLOCAL;          //控制模式，保证程序不会成为端口的占有者
    termios_new.c_cflag |= CREAD;           //控制模式，使能端口读取输入的数据

    // 控制模式，flow control
    fctl = pportinfo-> fctl;
    switch(fctl)
    {
    case '0':
    {
        termios_new.c_cflag &= ~CRTSCTS;        //no flow control
    }
    break;
    case '1':
    {
        termios_new.c_cflag |= CRTSCTS;         //hardware flow control
    }
    break;
    case '2':
    {
        termios_new.c_iflag |= IXON | IXOFF | IXANY; //software flow control
    }
    break;
    }

    //控制模式，data bits
    termios_new.c_cflag &= ~CSIZE;      //控制模式，屏蔽字符大小位
    databit = pportinfo -> databit;
    switch(databit)
    {
    case '5':
        termios_new.c_cflag |= CS5;
    case '6':
        termios_new.c_cflag |= CS6;
    case '7':
        termios_new.c_cflag |= CS7;
    default:
        termios_new.c_cflag |= CS8;
    }

    //控制模式 parity check
    parity = pportinfo -> parity;
    switch(parity)
    {
    case '0':
    {
        termios_new.c_cflag &= ~PARENB;     //no parity check
    }
    break;
    case '1':
    {
        termios_new.c_cflag |= PARENB;      //odd check
        termios_new.c_cflag &= ~PARODD;
    }
    break;
    case '2':
    {
        termios_new.c_cflag |= PARENB;      //even check
        termios_new.c_cflag |= PARODD;
    }
    break;
    }

    //控制模式，stop bits
    stopbit = pportinfo -> stopbit;
    if(stopbit == '2')
    {
        termios_new.c_cflag |= CSTOPB;  //2 stop bits
    }
    else
    {
        termios_new.c_cflag &= ~CSTOPB; //1 stop bits
    }

    //other attributions default
    termios_new.c_oflag &= ~OPOST;          //输出模式，原始数据输出
    termios_new.c_cc[VMIN]  = 1;            //控制字符, 所要读取字符的最小数量
    termios_new.c_cc[VTIME] = 1;            //控制字符, 读取第一个字符的等待时间    unit: (1/10)second

    termios_new.c_lflag &= ~(ICANON | ECHO | ECHOE);//lsw 不经处理直接发送,不需要回车和换行发送
    tcflush(fdcom, TCIFLUSH);               //溢出的数据可以接收，但不读
    tmp = tcsetattr(fdcom, TCSANOW, &termios_new);  //设置新属性，TCSANOW：所有改变立即生效    tcgetattr(fdcom, &termios_old);
    return(tmp);
}

/*******************************************
 *  Open serial port
 *  tty: 端口号 ttyS0, ttyS1, ....
 *  返回值为串口文件描述符
********************************************/
int PortOpen(pportinfo_t pportinfo)
{
    int fdcom;  //串口文件描述符
    char *ptty;

    ptty = get_ptty(pportinfo);

    /*
    O_NONBLOCK和O_NDELAY所产生的结果都是使I/O变成非搁置模式(non-blocking)，在读取不到数据或是写入缓冲区已满会马上return，而不会搁置程序动作，直到有数据或写入完成。


    不过需要注意的是，在GNU C中O_NDELAY只是为了与BSD的程序兼容，实际上是使用O_NONBLOCK作为宏定义，而且O_NONBLOCK除了在ioctl中使用，还可以在open时设定。

    APPENDED:
    如果没有数据，那么该调用将被阻塞.处于等待状态，直到有字符输入，
        或者到了规定的时限和出现错误为止,
        通过以下方法，能使read函数立即返回。

        fcntl(fd,F_SETFL,FNDELAY);

        FNDELAY 函数使read函数在端口没月字符存在的情况下，立刻返回0,
        如果要恢复正常(阻塞)状态,可以调用fcntl()函数，不要FNDELAY参数,
        如下所示：
            fcntl(Fd,F_SETFL,0);
        在使用O_NDELAY参数打开串行口后，同样与使用了该函数调用。

        fcntl(fd,F_SETFL,0);

    */

    //fdcom = open(ptty, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
    fdcom = open(ptty, O_RDWR | O_NOCTTY/* |O_NDELAY| O_NONBLOCK*/);

    return (fdcom);
}

/*******************************************
 *  Close serial port
********************************************/
void PortClose(int fdcom)
{
    close(fdcom);
}

/********************************************
 *  send data
 *  fdcom: 串口描述符，data: 待发送数据，datalen: 数据长度
 *  返回实际发送长度
*********************************************/
int PortSend(int fdcom, char *data, int datalen)
{
    int len = 0;

    len = write(fdcom, data, datalen);  //实际写入的长度
    if(len == datalen)
    {
        return (len);
    }
    else
    {
        tcflush(fdcom, TCOFLUSH);
        return -1;
    }
}

/*******************************************
 *  receive data
 *  返回实际读入的字节数
 *
********************************************/
int PortRecv(int fdcom, char *data, int datalen, int baudrate)
{
    int readlen, fs_sel;
    fd_set  fs_read;
    struct timeval tv_timeout;

    int flags;//lsw

    //C/C++ code
    flags = fcntl( fdcom, F_GETFL, 0 );
    fcntl( fdcom, F_SETFL, flags | O_NONBLOCK );
    FD_ZERO(&fs_read);
    FD_SET(fdcom, &fs_read);
    tv_timeout.tv_sec = TIMEOUT_SEC(datalen, baudrate);
    tv_timeout.tv_usec = TIMEOUT_USEC;

    fs_sel = select(fdcom + 1, &fs_read, NULL, NULL, &tv_timeout);
    if(fs_sel)
    {

        readlen = read(fdcom, data, datalen);
        return(readlen);
    }
    else
    {
        return(-1);
    }

    return (readlen);
}

#if 1
void read_com(int fdcom, int len)
{
    int RecvLen, i;
    char *RecvBuf = (char *)malloc(len);
    fcntl(fdcom, F_SETFL, 0); //阻塞
    char tmp = 0;
    unsigned int count = 0;
    for(;;)
    {

        RecvLen = read(fdcom, RecvBuf, len);
        //printf("RecvLen %d \n\n", RecvLen);
        if(RecvLen > 0)
        {
            for(i = 0; i < RecvLen; i++)
            {

                if(i == 0)
                {
                    goto next;//continue;
                }

                if(RecvBuf[i] != ((tmp + 1) % 10))
                {
                    printf("Error: crc err.\n");
                    return;

                }

next:
                //printf("%d\n",RecvBuf[i]);
                //printf("tmp %d\n",tmp);
                tmp = RecvBuf[i];
                count ++;

                if(count % (1024) == 0 )
                    printf("%dKB\n", count / (1024));

                // printf("%d ", count);
            }

        }
        else
        {
            printf("Error: receive error.\n");
        }
    }
}
#endif

int main(int argc, char *argv[])
{
    int fdcom, i, SendLen, RecvLen, baud, len;
    char port;
    struct termios termios_cur;
    char RecvBuf[128];
    portinfo_t portinfo =
    {
        '0',                            // print prompt after receiving
        57600/*115200*/,                // baudrate: 9600
        '8',                            // databit: 8
        '0',                            // debug: off
        '0',                            // echo: off
        '0',                            // flow control: software
        '3',                            // default tty: COM1
        '0',                            // parity: none
        '1',                            // stopbit: 1
        0                          	// reserved
    };

    port = *argv[2];
    baud = strtoul(argv[3], 0, 0);
    printf("port = %c\n", port);
    printf("baud = %d\n", baud);

    portinfo.baudrate = baud;
    portinfo.tty = port;



    fdcom = PortOpen(&portinfo);
    if(fdcom < 0)
    {
        printf("Error: open serial port error.\n");
        exit(1);
    }

    PortSet(fdcom, &portinfo);

    if(!strcmp(argv[1] , "w"))
    {
        printf("send char = %s\n", argv[4]);
        len = strtoul(argv[5], 0, 0);
        printf("len = %d\n", len);

        SendLen = PortSend(fdcom, argv[4], len);

    }
    else
    {
        len = strtoul(argv[4], 0, 0);
        printf("len = %d\n", len);
        read_com(fdcom, len);


    }
    PortClose(fdcom);
    return 0;
}

#if 0
//*************************Test*********************************
int main(int argc, char *argv[])
{
    int fdcom, i, SendLen, RecvLen;
    struct termios termios_cur;
    char RecvBuf[128];
    portinfo_t portinfo =
    {
        '0',                            // print prompt after receiving
        57600/*115200*/,                         // baudrate: 9600
        '8',                            // databit: 8
        '0',                            // debug: off
        '0',                            // echo: off
        '0',                            // flow control: software
        '3',                            // default tty: COM1
        '0',                            // parity: none
        '1',                            // stopbit: 1
        0                          	// reserved
    };


    fdcom = PortOpen(&portinfo);
    if(fdcom < 0)
    {
        printf("Error: open serial port error.\n");
        exit(1);
    }

    PortSet(fdcom, &portinfo);

    if(atoi(argv[1]) == 0)
    {
        //send data

        for(i = 0; i < 100; i++)
        {
            SendLen = PortSend(fdcom, "1234567890", 10);
            if(SendLen > 0)
            {
                printf("No %d send %d data 1234567890.\n", i, SendLen);
            }
            else
            {
                printf("Error: send failed.\n");
            }
            sleep(1);
        }
        PortClose(fdcom);

    }
    else
    {


        for(;;)
        {
            RecvLen = PortRecv(fdcom, RecvBuf, 10, portinfo.baudrate);

            if(RecvLen > 0)
            {
                for(i = 0; i < RecvLen; i++)
                {
                    printf("Receive data No %d is %x.\n", i, RecvBuf[i]);

                }
                printf("Total frame length is %d.\n", RecvLen);

            }
            else
            {
                printf("Error: receive error.\n");
            }
            sleep(2);//2000ms
        }




    }
    return 0;
}
#endif
