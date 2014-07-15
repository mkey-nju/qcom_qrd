


#include "lidbg.h"


int thread_bp_msg(void *data);

int start_index = 0;
int end_index = 0;
smem_log_deep *smem_log_temp = NULL;

int thread_bp_msg(void *data)
{
    int bp_msg_en;
    FS_REGISTER_INT(bp_msg_en, "bp_msg_en", 1, NULL);

    smem_log_temp->write_flag = 1;

    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
#if 0
        {
            start_index = smem_log_temp->start_pos;
            end_index = smem_log_temp->end_pos;
            if((end_index + 1) % 100 != start_index)
            {
                if((end_index + 1) < TOTAL_LOGS)
                {
                    lidbg("%s\n", smem_log_temp->log[end_index]);
                    smem_log_temp->log[end_index][0] = '\0';
                    smem_log_temp->end_pos++;
                }
                else if((end_index + 1) == TOTAL_LOGS)
                {
                    smem_log_temp->end_pos = 0;
                    lidbg("%s\n", smem_log_temp->log[end_index]);
                    smem_log_temp->log[end_index][0] = '\0';
                    smem_log_temp->end_pos++;
                }
                msleep(50);
            }
            else
            {
                msleep(50);
            }


        }
#endif
        {
            start_index = smem_log_temp->start_pos;
            end_index = smem_log_temp->end_pos;

            if(start_index != end_index)
            {
                if(bp_msg_en)lidbg("[lidbg_bp] %s\n", smem_log_temp->log[end_index]);
                smem_log_temp->end_pos = (end_index + 1)  % TOTAL_LOGS;
                msleep(BP_MSG_POLLING_TIME);
            }
            else
            {
                msleep(BP_MSG_POLLING_TIME);
            }

        }
    }
    return 0;
}



int bp_msg_init(void)
{
    DUMP_FUN;
    lidbg("\n[futengfei]  =bp_msg_init=IN===============================\n");
    smem_log_temp = (smem_log_deep *)smem_alloc(SMEM_ID_VENDOR1, sizeof(smem_log_deep));
    if(smem_log_temp == NULL)
    {
        lidbg("smem_alloc fail!\n");
        return 0;
    }

    CREATE_KTHREAD(thread_bp_msg, NULL);
    lidbg("[futengfei]  =bp_msg_init=OUT===============================\n");
    return 0;
}

void bp_msg_exit(void)
{
    lidbg("[futengfei]  ==OUT=================bp_msg_exit==============\n");
#if 0
    if(bp_msg_task)
    {
        kthread_stop(bp_msg_task);
        bp_msg_task = NULL;
    }
#endif
}


MODULE_AUTHOR("AILISHEN, <AILISHEN@AILISJIA.com>");
MODULE_DESCRIPTION("AP_REVEIVE_BP_MSG");
MODULE_LICENSE("GPL");

module_init(bp_msg_init);
module_exit(bp_msg_exit);

