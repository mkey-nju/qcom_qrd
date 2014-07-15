#ifndef _LIGDBG_I2C__
#define _LIGDBG_I2C__

int i2c_api_do_send(int bus_id, char chip_addr, unsigned int  sub_addr, char *buf, unsigned int size);
int i2c_api_do_recv(int bus_id, char chip_addr, unsigned int  sub_addr, char *buf, unsigned int size);
int i2c_api_do_recv_no_sub_addr(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int i2c_api_do_recv_sub_addr_2bytes(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int i2c_api_do_recv_sub_addr_3bytes(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);


int i2c_api_do_recv_SAF7741(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int i2c_api_do_send_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int i2c_api_do_recv_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
//add by huangozngqiang
int i2c_api_set_rate(int  bus_id, int rate);
void mod_i2c_main(int argc, char **argv);
extern int lidbg_i2c_running;

#endif

