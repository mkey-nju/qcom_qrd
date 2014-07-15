#include "lidbg.h"

static BLOCKING_NOTIFIER_HEAD(lidbg_chain_head);

int register_lidbg_notifier(struct notifier_block *nb)
{
    DUMP_FUN;
    return blocking_notifier_chain_register(&lidbg_chain_head, nb);
}

int unregister_lidbg_notifier(struct notifier_block *nb)
{
    DUMP_FUN;
    return blocking_notifier_chain_unregister(&lidbg_chain_head, nb);
}

int lidbg_notifier_call_chain(unsigned long val)
{
    int ret;
    lidbg("lidbg_notifier_call_chain:val=%ld,major=%ld,minor=%ld\n", val, val >> 16, val & 0xffff);
    ret = blocking_notifier_call_chain(&lidbg_chain_head, val, NULL);

    return notifier_to_errno(ret);
}

EXPORT_SYMBOL(register_lidbg_notifier);
EXPORT_SYMBOL(unregister_lidbg_notifier);
EXPORT_SYMBOL(lidbg_notifier_call_chain);


