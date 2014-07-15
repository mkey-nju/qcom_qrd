#ifndef _LIGDBG_UART__
#define _LIGDBG_UART__


void soc_io_uart_cfg(u32 baud);
void soc_io_uart_send(u32 baud, char *printChar);
void lidbg_uart_main(int argc, char **argv);
void soc_io_uart_send_byte(u8 input);

#endif
