#ifndef _LIGDBG_MEM_LOG__
#define _LIGDBG_MEM_LOG__


struct lidbg_fifo_device
{
    struct kfifo fifo;
    struct semaphore sem;
    char *owner;
    char *msg_in_buff;
    bool is_inited;
};


extern struct lidbg_fifo_device *glidbg_msg_fifo;
extern int lidbg_fifo_put( struct lidbg_fifo_device *dev, const char *fmt, ... );
extern int lidbg_fifo_get(struct lidbg_fifo_device *dev, char *to_file, int out_mode);
extern struct lidbg_fifo_device *lidbg_fifo_alloc(char *owner, int fifo_size,int buff_size);
extern void mem_log_main(int argc, char **argv);


#endif
