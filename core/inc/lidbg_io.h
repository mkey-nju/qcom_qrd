#ifndef _LIGDBG_IO__
#define _LIGDBG_IO__


typedef irqreturn_t (*pinterrupt_isr)(int irq, void *dev_id);

void mod_io_main(int argc, char **argv);

#endif

