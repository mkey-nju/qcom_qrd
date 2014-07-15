
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define MDBG(msg...) printf("[futengfei]server:"msg);
#define PORTNUM 3333
#define BUFFER_SIZE                1024
#define FILE_NAME_MAX_SIZE         512
static unsigned int server_count = 0;

int main()
{
    int sfp, nfp;
    int sin_size, n;
    struct sockaddr_in s_add, c_add;

    MDBG("Hello,welcome to my server !\n");
    sfp = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sfp)
    {
        MDBG("socket fail ! \n");
        return -1;
    }
    MDBG("socket ok !\n");//S1

    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = htonl(INADDR_ANY);
    s_add.sin_port = htons(PORTNUM);

    if(-1 == bind(sfp, (struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
    {
        MDBG("bind fail !\n");
        return -1;
    }
    MDBG("bind ok !\n");//S2

    if(-1 == listen(sfp, 20))
    {
        MDBG("listen fail !\n");
        return -1;
    }
    MDBG("listen ok\r\n");//S3
    while(1)//server should always in operation
    {
        sin_size = sizeof(struct sockaddr_in);
        server_count++;
        MDBG("\n\n[%dwaiting client connect.....]\n", server_count);
        nfp = accept(sfp, (struct sockaddr *)(&c_add), &sin_size);
        if(-1 == nfp)
        {
            MDBG("accept fail !\r\n");
            return -1;
        }
        MDBG("accept ok!\n");//S4
        MDBG("Server start get connect from %#x : %#x\r\n", ntohl(c_add.sin_addr.s_addr), ntohs(c_add.sin_port));

        //start to get data from client;
        {
            //start
            char buffer[BUFFER_SIZE];
            char dirname[BUFFER_SIZE];
            char filename[BUFFER_SIZE];

            int received_count = 0;//to check file length
            int write_length = 0;

            struct sockaddr_in client_addr;
            socklen_t length = sizeof(client_addr);

            //first,we should get the dirname.It is composed of [machine_id&&upload_date]
            bzero(dirname, sizeof(dirname));
            length = recv(nfp, dirname, BUFFER_SIZE, 0);
            if (length < 0)
            {
                MDBG("Server Recieve Data Failed!\n");
                break;
            }
            MDBG("%d,dirname:%s\n", length, dirname);

            //second,we will get the filename
            bzero(filename, sizeof(filename));
            length = recv(nfp, filename, BUFFER_SIZE, 0);
            if (length < 0)
            {
                MDBG("Server Recieve Data Failed!\n");
                break;
            }
            MDBG("%d,filename:%s\n", length, filename);

            //mkdir
            sprintf(buffer, "mkdir /data/lidbg_upload_history/%s", dirname);
            system(buffer);
            sprintf(buffer, "/data/lidbg_upload_history/%s", dirname);

            //then,get the file's absolutely path
            char file_name[FILE_NAME_MAX_SIZE + 1];
            bzero(file_name, sizeof(file_name));
            sprintf(file_name, "%s/%s", buffer, filename);

            MDBG("start to write file:%s\n", file_name);

            //The final step is to:creat it and write the stream to file.
            FILE *fp = fopen(file_name, "a");
            if (fp == NULL)
            {
                MDBG(" Can Not Open To Write!File:\t%s\n", file_name);
                exit(1);
            }

            bzero(buffer, BUFFER_SIZE);
            while((length = recv(nfp, buffer, BUFFER_SIZE, 0)) > 0)
            {
                write_length = fwrite(buffer, sizeof(char), length, fp);
                if (write_length < length)
                {
                    MDBG("File:\t%s Write Failed!\n", file_name);
                    break;
                }
                bzero(buffer, BUFFER_SIZE);
                //MDBG("server-pc===================%d\n", write_length);
                received_count += write_length;
            }

            MDBG("%d,Recieve File:\t [%s] Finished!\n", received_count, file_name);
            fclose(fp);
            fp = NULL;//note:To prevent the free pointer

        }//end


        close(nfp);
    }
    close(sfp);
    return 0;
}
