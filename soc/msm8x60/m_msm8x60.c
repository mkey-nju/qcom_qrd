/*======================================================================
======================================================================*/

#include "lidbg.h"

struct fly_smem *p_fly_smem = NULL;


int soc_temp_get(void)
{
    return 0;
}


int msm8x25_init(void)
{


    return 0;
}

/*Ä£¿éÐ¶ÔØº¯Êý*/
void msm8x25_exit(void)
{
    lidbg("msm8x25_exit\n");

}

void lidbg_soc_main(int argc, char **argv)
{
    lidbg("lidbg_soc_main\n");

    if(argc < 3)
    {
        lidbg("Usage:\n");
        return;
    }
}


EXPORT_SYMBOL(lidbg_soc_main);
EXPORT_SYMBOL(p_fly_smem);
EXPORT_SYMBOL(soc_temp_get);

MODULE_AUTHOR("Lsw");
MODULE_LICENSE("GPL");

module_init(msm8x25_init);
module_exit(msm8x25_exit);
