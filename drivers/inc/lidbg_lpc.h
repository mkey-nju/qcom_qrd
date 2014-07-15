#ifndef __FLY_LPC_
#define __FLY_LPC_


#define LPC_CMD_LCD_ON  do{    \
		u8 buff[] = {0x02, 0x0d, 0x1};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_LCD_OFF   do{   \
		u8 buff[] = {0x02, 0x0d, 0x0};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)

#define LPC_CMD_NO_RESET   do{   \
		u8 buff[] = {0x00,0x03,0x01,0xff};\
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));\
				}while(0)


#endif
