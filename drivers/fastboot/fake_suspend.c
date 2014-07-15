

static DEFINE_MUTEX(early_suspend_lock);
static LIST_HEAD(early_suspend_handlers);

/*
struct early_suspend {
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct list_head link;
	int level;
	void (*suspend)(struct early_suspend *h);
	void (*resume)(struct early_suspend *h);
#endif
};
*/
//typedef void (*fake_suspend_func)(struct early_suspend *h);
//fake_suspend_func pfunc_suspend [10]= {NULL} ;
//fake_suspend_func pfunc_wakeup [10]= {NULL} ;

struct early_suspend *fake_suspend_handler[10] = {NULL} ;

void fake_register_early_suspend(struct early_suspend *handler)
{
#if 0
    struct list_head *pos;

    mutex_lock(&early_suspend_lock);
    list_for_each(pos, &early_suspend_handlers)
    {
        struct early_suspend *e;
        e = list_entry(pos, struct early_suspend, link);
        if (e->level > handler->level)
            break;
    }
    list_add_tail(&handler->link, pos);
    mutex_unlock(&early_suspend_lock);

#else
    int i;
    for(i = 0; i < 10; i ++)
    {
        if(fake_suspend_handler[i] == NULL)
        {
            fake_suspend_handler[i] = handler;
            break;
        }
    }


#endif
}


static void fake_early_suspend(void)
{
#if 0
    struct early_suspend *pos;
    list_for_each_entry(pos, &early_suspend_handlers, link)
    {
        if (pos->suspend != NULL)
        {

            lidbg("early_suspend: calling %pf\n", pos->suspend);
            pos->suspend(pos);
        }
    }

#else
    int i;
    for(i = 0; i < 10; i ++)
    {
        if(fake_suspend_handler[i] != NULL)
        {
            fake_suspend_handler[i]->suspend(fake_suspend_handler[i]);
        }
    }



#endif
}

static void fake_late_resume(void)
{
#if 0
    struct early_suspend *pos;

    list_for_each_entry_reverse(pos, &early_suspend_handlers, link)
    {
        if (pos->resume != NULL)
        {
            lidbg("late_resume: calling %pf\n", pos->resume);
            pos->resume(pos);
        }
    }

#else

    int i;
    for(i = 0; i < 10; i ++)
    {
        if(fake_suspend_handler[i] != NULL)
        {
            fake_suspend_handler[i]->resume(fake_suspend_handler[i]);
        }
    }


#endif
}




int fake_suspend(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{

    DUMP_FUN_ENTER;
    if(is_fake_suspend == 0)
    {
        is_fake_suspend = 1;

        fastboot_set_status(PM_STATUS_READY_TO_FAKE_PWROFF);

        fake_early_suspend();
        //  fastboot_task_kill_exclude(kill_exclude_process_fake_suspend);

        //SOC_Write_Servicer(SUSPEND_KERNEL);
    }
    DUMP_FUN_LEAVE;

    return sprintf(buf, "is_fake_suspend:%d\n", is_fake_suspend);
}

void create_proc_entry_fake_suspend(void)
{
    create_proc_read_entry("fake_suspend", 0, NULL, fake_suspend, NULL);

}

int fake_wakeup(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{

    DUMP_FUN_ENTER;
    if(is_fake_suspend == 1)
    {
        is_fake_suspend = 0;
        fake_late_resume();
        //SOC_Write_Servicer(WAKEUP_KERNEL);
    }
    DUMP_FUN_LEAVE;

    return sprintf(buf, "is_fake_suspend:%d\n", is_fake_suspend);

}

void create_proc_entry_fake_wakeup(void)
{
    create_proc_read_entry("fake_wakeup", 0, NULL, fake_wakeup, NULL);

}




