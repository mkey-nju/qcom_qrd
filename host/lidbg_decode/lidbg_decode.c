
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/utsname.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/inotify.h>

#define MDBG(msg...) printf("[futengfei]decode:"msg);

void mem_decode(char *addr, int length, int step_size)
{
    int loop = 0;
    char temp;
    while(loop < length)
    {
        temp = *(addr + loop) + step_size;
        if(temp > 0 && temp < 127)
            *(addr + loop) = temp;
        loop++;
    }
}

char *get_file_content(char *filename, int *file_len)
{
    int fd;
    char *read_buf;
    long unsigned fileSize;
    struct stat st;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        goto err_open;

    if (fstat(fd, &st) < 0)
        goto err_fstat;

    fileSize = st.st_size;
    if(file_len)
        *file_len = fileSize;

    read_buf = (char *)malloc(fileSize);
    memset(read_buf, 0x0, fileSize);
    read(fd, read_buf, fileSize);
    close(fd);
    return read_buf;
err_open:
    MDBG("get_file_content:%s\n", filename);
err_fstat:
    MDBG("fstat:%s\n", filename);
    close(fd);
    return NULL;
}

void set_file_content(char *filename, char *content, int content_len)
{
    FILE *fp = fopen(filename, "a");
    if (fp == NULL)
    {
        MDBG(" Can Not Open To Write!File:%s\n", filename);
        return;
    }
    fwrite(content, sizeof(char), content_len, fp);
    fclose(fp);
}

bool decode_one_file(char *inputfile, char *outputfile)
{
    char *filecontent = NULL;
    int filelen = 0;

    filecontent = get_file_content(inputfile, &filelen);
    if(filecontent == NULL || filelen == 0)
    {
        MDBG(" get_file_content:%s\n", inputfile);
        return false;
    }
    mem_decode(filecontent, filelen, -1);
    set_file_content(outputfile, filecontent, filelen);
    free(filecontent);
    return true;
}

void opendir_and_decode(char *dir_input, char *dir_output)
{
    struct dirent *entry;
    DIR *dir;
    char input_filen_ame[512] = {0};
    char output_filen_ame[512] = {0};
    int decode_count = 0;
    if (access(dir_input, R_OK) != 0)
    {
        MDBG("dir miss:[%s]  return;\n", dir_input);
        return ;
    }
    dir = opendir(dir_input);
    if (dir == NULL)
    {
        MDBG("Could not open %s\n", dir_input);
        return ;
    }

    while ((entry = readdir(dir)))
    {
        const char *name = entry->d_name;
        if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0)))
            continue;
        decode_count++;
        sprintf(output_filen_ame, "%s/%s", dir_output, entry->d_name);
        sprintf(input_filen_ame, "%s/%s", dir_input, entry->d_name);
        ;
        MDBG("lidbg_decode:%d = %s------[%s][%s]\n", decode_count,decode_one_file(input_filen_ame, output_filen_ame)?"success":"fail", input_filen_ame, output_filen_ame);
    }

    closedir(dir);
}
int main(int argc, char **argv)
{
    char outdir[64] = {0};
    char cmd[128] = {0};
    if (argc < 2)
    {
        MDBG("usage: ./lidbg_decode <dirname>\n");
        return -1;
    }
    sprintf(outdir, "%s_decode", argv[1]);
    sprintf(cmd, "mkdir  %s", outdir);
    system(cmd);

    opendir_and_decode(argv[1], outdir);

}
