#ifndef _LIGDBG_NOTIFIER__
#define _LIGDBG_NOTIFIER__

#define NOTIFIER_VALUE(major,minor)  (((major)&0xffff)<<16 | ((minor)&0xffff))

int register_lidbg_notifier(struct notifier_block *nb);
int unregister_lidbg_notifier(struct notifier_block *nb);
int lidbg_notifier_call_chain(unsigned long val);

#endif
