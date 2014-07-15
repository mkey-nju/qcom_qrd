#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

void *get_mmap()
{

    unsigned long phymem_addr, phymem_size , valid_offset;
    char *map_addr;
    char s[256];
    int fd;

    if( (fd = open("/proc/shm_dir/shm_info", O_RDONLY)) < 0)
    {
        lidbg("cannot open file /proc/shm_dir/shm_info\n");
        return NULL;
    }
    read(fd, s, sizeof(s));
    if(3 != sscanf(s, "%lx %lu %lu", &phymem_addr, &phymem_size, &valid_offset))
    {
        lidbg("data format from /proc/shm_dir/shm_info error!\n");
        close(fd);
        return NULL;
    };
    close(fd);
    lidbg("phymem_addr=%lx,phymem_size=%lu,offset=%lu\n", phymem_addr, phymem_size, valid_offset);

#if 0
    if( (fd = open("/dev/mem", O_RDWR | O_NONBLOCK)) < 0)
    {
        lidbg("open /dev/mem error!\n");
        return NULL;
    }
    map_addr = mmap(NULL,
                    phymem_size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd,
                    phymem_addr);
#else

    if( (fd = open("/dev/lidbg_share", O_RDWR | O_NONBLOCK)) < 0)
    {
        lidbg("open /dev/lidbg_share error!\n");
        return -1;
    }

    map_addr = mmap(NULL,
                    phymem_size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd,
                    0); // ignored, because we don't use "vm->pgoff" in driver.

    if( MAP_FAILED == map_addr)
    {
        lidbg("mmap() error:[%d]\n", errno);
        return -1;
    }


#endif

    close(fd);
    map_addr = map_addr + valid_offset;

    return (void *)map_addr;
}
