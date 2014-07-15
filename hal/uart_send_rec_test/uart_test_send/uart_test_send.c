#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

/*******************************************
 *  ??????
********************************************/
#define TTY_DEV "/dev/tcc-uart"

char *get_ptty(unsigned int port)
{
    char *ptty = NULL;

    switch(port)
    {
    case 0:
    {
        ptty = TTY_DEV"0";
    }
    break;
    case 1:
    {
        ptty = TTY_DEV"1";
    }
    break;
    case 2:
    {
        ptty = TTY_DEV"2";
    }
    break;
    case 3:
    {
        ptty = TTY_DEV"3";
    }
    break;
    case 4:
    {
        ptty = TTY_DEV"4";
    }
    break;
    case 5:
    {
        ptty = TTY_DEV"5";
    }
    break;
    }
    return(ptty);
}

/*******************************************
 *  ???????(???????)
********************************************/
int convbaud(unsigned int baudrate)
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



//./uart_test 3 38400

int main(int argc , char **argv)
{
    int fd, i, len , sendlen = 0;
    //unsigned char buff[1024];
    struct termios oldtio;
    unsigned int port, baud;
    char *ptty;
    unsigned int count = 0;
    char *buff;
    //while(1)
    //{
    // usleep(1000 * 10); //100ms
    //printf(".");
    //}

    //////////////////////////////
    port = strtoul(argv[1], 0, 0);
    baud = strtoul(argv[2], 0, 0);
    len = strtoul(argv[3], 0, 0);

    printf("port = %d\n", port);
    printf("baud = %d\n", baud);
    printf("len = %d\n", len);
    //if(!strcmp(argv[1]) ,"on")
    //////////////////////////////


    ptty = get_ptty(port);
    printf("ptty = %s\n", ptty);

    fd = open( ptty, O_RDWR | O_NOCTTY/*|O_NDELAY*/);
    if (fd == (-1))
    {
        perror("Can't Open Uart\n");
        return 0;
    }
    else
        printf("Open Uart Success22 !\n");


    if (tcgetattr(fd, &oldtio) != 0)
    {
        perror("tcgetattr fail!\n");
        return 0;
    }

    cfsetispeed(&oldtio, convbaud(baud));
    cfsetospeed(&oldtio, convbaud(baud));

    oldtio.c_cflag |= CS8;//8 bit
    oldtio.c_cflag &= ~CSTOPB;// 1 stop
    oldtio.c_cflag &= ~PARENB;//N
    oldtio.c_lflag &= ~(ICANON | ECHO | ECHOE);//


    tcflush(fd, TCIFLUSH);
    if((tcsetattr(fd, TCSANOW, &oldtio)) != 0)
    {
        perror("tcsetattr fail!\n");
        return 0;
    }


    buff = (char *)malloc(len);
    i = 0;
    while(i < len)
    {
        buff[i] = i % 10;
        i++;
    }


    printf("enter while(1)\n");
    while(1)
    {
        sendlen = write(fd, buff, len);
        //  printf("%d\n",sendlen);
        count = count + sendlen;

        if(count % (1024) < 100)
            printf("%dKB\n", count / (1024));


        usleep(1000 * 50); //100ms
        //printf(".\n");
    }

    close(fd);
    return 0;
}
