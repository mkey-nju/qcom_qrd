#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>s
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>

#define LOG_BYTES   (100)
#define LIDBG_PRINT(msg...) do{\
	int fd;\
	char s[LOG_BYTES];\
	sprintf(s, "lidbg_msg: " msg);\
	s[LOG_BYTES - 1] = '\0';\
	 fd = open("/dev/lidbg_msg", O_RDWR);\
	 if((fd == 0)||(fd == (int)0xfffffffe)|| (fd == (int)0xffffffff))break;\
	 write(fd, s, /*sizeof(msg)*/strlen(s)/*LOG_BYTES*/);\
	 close(fd);\
}while(0)

#define MDBG(msg...) printf("[futengfei]client_mobile:"msg)

#define BUFFER_SIZE                   1024
#define FILE_NAME_MAX_SIZE            512
#define PROTNO 3333
#define LIDBG_PATH "/data/lidbg"
#define FILE_UPLOAD_TMP "/data/lidbg_upload_history/tmp.tar.gz"
#define MACHINE_ID_FILE "MIF.txt"
static char g_date[32];
static char g_machine_id[32];
static char g_file_information[100];

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
void send_file_to_server(char *dir_name, char *file_name)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;

    char buffer[256];
    char *filepath[128];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LIDBG_PRINT("ERROR opening socket\n");
        system("echo ERROR opening socket > /dev/log/mobile.txt");
        error("ERROR opening socket\n");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    serv_addr.sin_addr.s_addr = inet_addr("192.168.9.222");
    serv_addr.sin_port = htons(PROTNO);

    sprintf(filepath, "%s/%s", dir_name, file_name);


    LIDBG_PRINT("send_file_to_server:%s\n", filepath);
    LIDBG_PRINT("start to connect sever.....\n");
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        LIDBG_PRINT("ERROR connecting\n");
        system("echo ERROR connecting > /dev/log/mobile.txt");
        error("ERROR connecting\n");
    }

    {
        //start
        char buffer[BUFFER_SIZE];
        bzero(buffer, sizeof(buffer));
        strncpy(buffer, g_file_information, strlen(g_file_information) > BUFFER_SIZE ? BUFFER_SIZE : strlen(g_file_information));
        send(sockfd, buffer, BUFFER_SIZE, 0);

        bzero(buffer, sizeof(buffer));
        strncpy(buffer, file_name, strlen(file_name) > BUFFER_SIZE ? BUFFER_SIZE : strlen(file_name));
        send(sockfd, buffer, BUFFER_SIZE, 0);

        LIDBG_PRINT("start to send file[%s]\n", buffer);

        FILE *fp = fopen(filepath, "r");
        if (fp == NULL)
        {
            LIDBG_PRINT("Fail Create:File:%s\n", filepath);
        }
        else
        {
            LIDBG_PRINT("Success Create:File:%s\n", filepath);
            bzero(buffer, BUFFER_SIZE);
            int file_block_length = 0;
            int send_count = 0;
            while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
            {
                if (send(sockfd, buffer, file_block_length, 0) < 0)
                {
                    LIDBG_PRINT("Send  Failed,File:%s\n", filepath);
                    break;
                }
                //MDBG("client-mobile===================%d\n", file_block_length);
                send_count += file_block_length;
                bzero(buffer, sizeof(buffer));
            }

            fclose(fp);
            fp = NULL;
            LIDBG_PRINT("%d,Transfer Finished:File:%s\n", send_count, filepath);
            system("echo success > /dev/log/mobile.txt");
        }

    }//end
    close(sockfd);
}
int read_from_file(const char *path, char *buf, size_t size)
{
    if (!path)
        return -1;
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1)
    {
        MDBG("Could not open '%s'\n", path);
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
int get_machine_id(char *machine_id, size_t size)
{
    char file_path[64];
    sprintf(file_path, "%s/%s", LIDBG_PATH, MACHINE_ID_FILE);
    int ok = read_from_file(file_path, machine_id, size) ;
    if(ok > 0)
        MDBG("succeeded in getting machine_id:%s\n", machine_id);
    else
        MDBG("failed in getting machine_id\n");
    return ok;
}
void get_time(char *datastring)
{
    struct tm *local;
    const char *tmFormat = "%Y%m%d_%I%M%S";
    time_t now = time(NULL);
    local = localtime(&now);
    if(datastring)
        strftime(datastring, 64, tmFormat, local);
    MDBG("get_time:%s\n", datastring);
}
void send_file_prepare(void)
{
    char cmd[256];
    get_machine_id(g_machine_id, sizeof(g_machine_id));
    get_time(g_date);
    sprintf(g_file_information, "%s_%s", g_machine_id, g_date);

    //system("mkdir /data/lidbg_upload_history");
    //sprintf(cmd, "busybox tar -zcf %s /data/*.txt", g_file_to_upload);
    //system(cmd);

}
void scan_dir_txtfile_and_send(char *dir_name)
{
    struct dirent *entry;
    DIR *dir = opendir(dir_name);
    if (dir == NULL)
    {
        MDBG("Could not open %s\n", dir_name);
    }
    else
    {
        while ((entry = readdir(dir)))
        {
            const char *file_name = entry->d_name;
            if (strstr(file_name, "txt"))
            {
                MDBG("will send:=========2==========:%s\n", file_name);
                send_file_to_server(dir_name, file_name);
            }

        }
    }
}
void send_file_clean_tmp(void)
{
    system("rm /data/lidbg_upload_history/*.tar.gz");
}
int main(int argc, char *argv[])
{
    send_file_prepare();
    scan_dir_txtfile_and_send("/data/lidbg");
    return 0;
}
