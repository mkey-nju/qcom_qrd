#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/resource.h>

#define DEBUG_PRINT_WRITE_FLAG 0
#define DEBUG_PRINT_READ_FLAG 0
#define DEBUG_COUNT_FLAG 1
#define DEBUG_CRC_FLAG 1
#define RD_SIZE (250)

struct uartConfig
{
    int fd;
    int ttyReadWrite;
    int baudRate;
    char *portName;

    int nread;	//bytes to read
    int nwrite;	//bytes to write
    int wlen;		//lenght have sent
    int rlen;		//lenght have read
};

static struct uartConfig pUartInfo;

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio, oldtio;

    if ( tcgetattr( fd, &oldtio) != 0)
    {
        printf("SetupSerial .\n");
        return -1;
    }

    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch ( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch ( nEvent )
    {
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    }

    switch ( nSpeed )
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;
    case 576000:
        cfsetispeed(&newtio, B576000);
        cfsetospeed(&newtio, B576000);
        break;
    case 921600:
        cfsetispeed(&newtio, B921600);
        cfsetospeed(&newtio, B921600);
        break;
    case 3000000:
        cfsetispeed(&newtio, B3000000);
        cfsetospeed(&newtio, B3000000);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }

    if ( nStop == 1 )
        newtio.c_cflag &= ~CSTOPB;
    else if ( nStop == 2 )

        newtio.c_cflag |= CSTOPB;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
    {
        printf("com set error\n");
        return -1;
    }

    printf("set done!\n");
    return 0;
}

int open_port(int fd, char *portName)
{
    long vdisable;

    pUartInfo.fd = open( portName, O_RDWR | O_NOCTTY);

    if (-1 == pUartInfo.fd)
    {
        printf("Can't Open Serial Port\n");
        return(-1);
    }
    else
    {
        printf("open %s .....\n", portName);
    }

    if (fcntl(pUartInfo.fd, F_SETFL, 0) < 0)
        printf("fcntl failed!\n");
    else
        printf("fcntl=%d\n", fcntl(pUartInfo.fd, F_SETFL, 0));
    if (isatty(STDIN_FILENO) == 0)
        printf("standard input is not a terminal device\n");
    else
        printf("isatty success!\n");

    return pUartInfo.fd;
}

void *write_thread_fun(void *arg)
{
    int i = 0;

    struct uartConfig *pInfo = arg;
    static unsigned char wbuff[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    pInfo->nwrite = sizeof(wbuff);

    while(1)
    {
        pInfo->wlen = 0;

        if((pInfo->fd) > 0) //Tx
        {
            for(i = 0; i < 50; i++)
                pInfo->wlen = write(pInfo->fd, wbuff, pInfo->nwrite);

            //usleep(10 * 1000);
#if DEBUG_PRINT_WRITE_FLAG
            printf("sent:%d  %s\n", pInfo->wlen, wbuff);
#endif
        }

    }

    return NULL;
}

void *read_thread_fun(void *arg)
{

    int i = 0;
    int ret = -1;
    int timesFlag = 0;
    char tmp = 0;
    char parm = 0;
    unsigned int count = 0;
    struct uartConfig *pInfo = arg;
    static unsigned char *rbuff;

    pInfo->nread = RD_SIZE;

    rbuff = (unsigned char *)malloc(RD_SIZE * sizeof(char));
    if(rbuff == NULL)
    {
        printf("Alloc mem fo rbuff failed.");
        return NULL;
    }

    memset(rbuff, 0, RD_SIZE * sizeof(char));

    while(1)
    {
        pInfo->rlen = 0;

        if((pInfo->fd) > 0) 	//Rx
        {
            pInfo->rlen = read(pInfo->fd, rbuff, pInfo->nread);

            if(pInfo->rlen == 0) 	//in case of don't back to the start of the file, when reading to the end
            {
                lseek(pInfo->fd, 0, SEEK_SET);
                continue;
            }

            if(pInfo->rlen > 0)
            {

#if DEBUG_PRINT_READ_FLAG
                for(i = 0; i < pInfo->rlen; i++)
                {
                    printf("%d", rbuff[i]);
                }
                printf("\n");
#endif
#if DEBUG_CRC_FLAG
                tmp = rbuff[0];

                if(timesFlag != 0)
                    if(tmp != ((parm + 1) % 10))
                    {
                        printf("Received error at the next time ...\n");

                        printf("%d - %d", parm, tmp);
                        printf("\n\n");

                        //						timesFlag = 0;
                        //						exit(0);
                    }

                for(i = 1; i < pInfo->rlen; i++)
                {

#if DEBUG_COUNT_FLAG
                    count ++;

                    if(count % (1024 * 1024) == 0 )
                        printf("%dMB\n", count / (1024 * 1024) );

#endif
                    if(rbuff[i] != ((tmp + 1) % 10))
                    {
                        printf("Received Error ...\n");

                        printf("%d - %d", tmp, rbuff[i]);
                        printf("\n\n");

                        //						timesFlag = 0;
                        //						exit(0);
                    }

                    tmp = rbuff[i];
                    timesFlag = 1;
                }

                parm = tmp;
#endif
            }
        }
    }

    return NULL;
}

int main(int argc , char **argv)
{

    static pthread_t readTheadId;
    static pthread_t writeTheadId;
    int ret;
    int i = 0;
    /*
    	setpriority(PRIO_PROCESS, getpid(), -20);
    	struct sched_param sp = {90};
    	if (sched_setscheduler(0,SCHED_FIFO, &sp) < 0)
    	{
    		printf(" Error !! \n");
    	}
    */
    if(argc < 3)
    {
        printf("uaer:%s TxRx(0) ttyNumber\n", argv[0]);
        printf("uaer:%s Tx(1) ttyNumber\n", argv[0]);
        printf("uaer:%s Rx(2) ttyNumber\n", argv[0]);
        printf("uaer:example: %s 0 /dev/ttyS0 115200\n", argv[0]);
        return -1;
    }

    pUartInfo.ttyReadWrite = strtoul(argv[1], 0, 0);
    pUartInfo.portName = argv[2];
    pUartInfo.baudRate = strtoul(argv[3], 0, 0);
    pUartInfo.fd = -1;

    if(pUartInfo.portName == NULL)
    {
        printf("ERR:tty dev does not exist.\n");
        return -1;
    }

    printf("%s %d %s %d\n", argv[0], pUartInfo.ttyReadWrite, pUartInfo.portName, pUartInfo.baudRate);

    if ((pUartInfo.fd = open_port(pUartInfo.fd, pUartInfo.portName)) < 0)
    {
        printf("open_port error\n");
        return -1;
    }

    if ((i = set_opt(pUartInfo.fd, pUartInfo.baudRate, 8, 'N', 1)) < 0)
    {
        printf("set_opt error\n");
        return -1;
    }

    //read thread
    if((pUartInfo.ttyReadWrite == 0) || (pUartInfo.ttyReadWrite == 2))
    {
        ret = pthread_create(&readTheadId, NULL, read_thread_fun, (void *)&pUartInfo);
        if(ret != 0)
        {
            printf("Failed to create open and close thrad !\n");
            return -1;
        }
    }
    else if(pUartInfo.ttyReadWrite != 1)
    {
        printf("Unknown test mode.\n");
        return -1;
    }

    //write thread
    if((pUartInfo.ttyReadWrite == 0) || (pUartInfo.ttyReadWrite == 1))
    {
        ret = pthread_create(&writeTheadId, NULL, write_thread_fun, (void *)&pUartInfo);
        if(ret != 0)
        {
            printf("Failed to create read and write thrad !\n");
            return -1;
        }
    }

    while(1)
        sleep(60);

    close(pUartInfo.fd);

    return 0;
}



