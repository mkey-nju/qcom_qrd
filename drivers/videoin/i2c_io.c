#include "lidbg.h"
#include "i2c_io.h"
#include <linux/gpio.h>
#include <linux/io.h>
#include"tw9912.h"
#include"TC358746XBG.h"
#include "lidbg_i2c.h" //add by huangzongqiang
#define I2C_IO_IMITATION1
#ifdef I2C_IO_IMITATION1
static struct mutex io_i2c_lock;
#define  Dtime 1
#define  DELAY_UNIT 1
extern struct lidbg_hal *plidbg_dev;//add by huangzongqiang
int i2c_io_config(unsigned int index, unsigned int direction, unsigned int pull, unsigned int drive_strength, unsigned int flag)
{
    int rc;
    int err;
    mutex_init(&io_i2c_lock);
    if(flag == 1)
    {
        /*	if (gpio_is_valid(index))
        	{
        		gpio_free(index);
        		lidbg("gpio_free(%d);\n",index);
        	}
        */
        err = gpio_request(index, "video_io");
        lidbg("gpio_request(%d)\n ", index);
        if (err)
        {
            lidbg("\n\nerr: gpio request failed!!!!!!\n\n\n");
            goto gpio_request_failed;
        }

        rc = gpio_tlmm_config(GPIO_CFG(index, 0,
                                       direction, pull,
                                       drive_strength), GPIO_CFG_ENABLE);
        if (rc)
        {
            lidbg("%s: gpio_tlmm_config for %d failed\n",
                   __func__, index);
            return 0;
        }

    }
    if(direction == GPIO_CFG_INPUT)
    {
        err = gpio_direction_input(index);
    }
    else
    {
        err = gpio_direction_output(index, 1);
    }

    if (err)
    {
        lidbg("gpio_direction_set failed\n");
        goto free_gpio;
    }
    return 1;
free_gpio:
    //if (gpio_is_valid(index))
    //	gpio_free(index);

    return 0;
gpio_request_failed:
    return 0;
}
#ifndef FLY_VIDEO_BOARD_V3
static void set_i2c_pin(u32 pin)
{
    //  soc_io_output(pin, 1);
    gpio_set_value(pin, 1);
}
static void clr_i2c_pin(u32 pin)
{
    //soc_io_output(pin,0);
    gpio_set_value(pin, 0);
}
#endif
void i2c_io_config_init(void)
{
#ifndef FLY_VIDEO_BOARD_V3
    i2c_io_config(GPIO_I2C_SDA, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 1);
    i2c_io_config(GPIO_I2C_SCL, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 1);
#endif
    i2c_io_config(TW9912_RESET, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 1); //tw9912 reset//43
    i2c_io_config(TC358746XBG_RESET, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 1); // tc358746 reset
    lidbg("first init GPIO_I2C_SDA and GPIO_I2C_SCL\n");
}
#ifndef FLY_VIDEO_BOARD_V3
static void i2c_init(void)
{

    i2c_io_config( GPIO_I2C_SDA, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 0);
    i2c_io_config( GPIO_I2C_SCL, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 0);

    set_i2c_pin(GPIO_I2C_SDA);
    set_i2c_pin(GPIO_I2C_SCL);

}

static void i2c_free(void)
{
    return;
}

static void i2c_delay(u32 time)
{
#if 0
    while(time--)
    {
        ;
    }
#else
    //udelay(time);
#endif
}
static void i2c_begin(void)
{
    clr_i2c_pin(GPIO_I2C_SDA);//start
    i2c_delay(DELAY_UNIT << Dtime);

}
static void i2c_stop(void)
{
    clr_i2c_pin(GPIO_I2C_SCL);
    clr_i2c_pin(GPIO_I2C_SDA);
    set_i2c_pin(GPIO_I2C_SCL);
    i2c_delay(DELAY_UNIT << Dtime);
    set_i2c_pin(GPIO_I2C_SDA);
    i2c_delay(DELAY_UNIT << Dtime);
}

static void i2c_write_ask(i2c_ack flag)
{
    if(flag == NACK)
        set_i2c_pin(GPIO_I2C_SDA);//nack
    else
        clr_i2c_pin(GPIO_I2C_SDA);//ack
    i2c_delay(DELAY_UNIT << Dtime);
    set_i2c_pin(GPIO_I2C_SCL);
    i2c_delay(DELAY_UNIT << Dtime);
    clr_i2c_pin(GPIO_I2C_SCL);
    i2c_delay(DELAY_UNIT << Dtime);
    set_i2c_pin(GPIO_I2C_SDA);
    i2c_delay(DELAY_UNIT << Dtime);
}


static i2c_ack i2c_read_ack(void)
{
    u8 ret;
    clr_i2c_pin(GPIO_I2C_SCL);//clk
    i2c_io_config( GPIO_I2C_SDA, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA, 0);
    i2c_delay(DELAY_UNIT << Dtime);

    set_i2c_pin(GPIO_I2C_SCL);
    i2c_delay(DELAY_UNIT << Dtime);

    if (!gpio_get_value(GPIO_I2C_SDA))//read ack
    {
        ret = ACK;
    }
    else
    {
        ret = NACK;
    }
    clr_i2c_pin(GPIO_I2C_SCL);//clk
    i2c_io_config( GPIO_I2C_SDA, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA, 0);



    return ret;
}
static u8 i2c_read(void)
{
    u8 i;
    u8 data = 0x0;
    i2c_io_config( GPIO_I2C_SDA,  GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 0);
    for (i = 0; i < 8; i++)
    {
        i2c_delay(DELAY_UNIT << Dtime);
        set_i2c_pin(GPIO_I2C_SCL);
        i2c_delay(DELAY_UNIT << Dtime);
        data = data << 1;
        if (gpio_get_value(GPIO_I2C_SDA))
            data |= 1;
        i2c_delay(DELAY_UNIT << Dtime);//clk
        clr_i2c_pin(GPIO_I2C_SCL);
        i2c_delay(DELAY_UNIT << Dtime);
        if (i == 7)
            i2c_io_config( GPIO_I2C_SDA,  GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA, 0);
        i2c_delay(DELAY_UNIT << Dtime);
    }

    return data;
}
static void i2c_write(u8 data)
{
    u8 i;
    //	i2c_io_config( GPIO_I2C_SDA,  GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA,0);
    //		 i2c_delay(DELAY_UNIT << 1);

    for (i = 0; i < 8; i++)
    {
        clr_i2c_pin(GPIO_I2C_SCL);
        i2c_delay(DELAY_UNIT << Dtime);
        if (data & 0x80)
            set_i2c_pin(GPIO_I2C_SDA);
        else
            clr_i2c_pin(GPIO_I2C_SDA);
        data <<= 1;

        i2c_delay(DELAY_UNIT << Dtime);//clk
        set_i2c_pin(GPIO_I2C_SCL);
        i2c_delay(DELAY_UNIT << Dtime);
    }
}
static void i2c_write_chip_addr(u8 chip_addr, i2c_WR_flag flag)
{
    if (flag == hkj_WRITE)
    {
        chip_addr = chip_addr << 1;
        i2c_write(chip_addr);
    }
    else//hkj_READ
    {
        chip_addr = chip_addr << 1;
        chip_addr = chip_addr | 0x01;
        //    lidbg("READ:i2c_write(%.2x)",chip_addr);
        i2c_write(chip_addr) ;
    }
}
#endif
i2c_ack i2c_read_byte(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
#ifdef FLY_VIDEO_BOARD_V3
    int ret = 0;
    //ret=i2c_api_do_recv(3,chip_addr, sub_addr , buf, size);
    ret = SOC_I2C_Rec(3, chip_addr, sub_addr , buf, size);
    //lidbg("\n************ i2c_read_byte =%d*******\n",ret);
    //if(ret==size){
    if(ret > 0)
    {
#ifdef  DEBUG_ACK
        return NACK;
#else
        return ACK;
#endif

    }
    else
    {
        return NACK;
    }
#else
    u8 i;
    mutex_lock(&io_i2c_lock);
    i2c_init();
    // start transmite
    i2c_begin();
    i2c_write_chip_addr(chip_addr, hkj_WRITE) ;
    if (i2c_read_ack () == NACK)
    {
        i2c_stop();
        i2c_free();
        lidbg("at read funtion :i2c_write_chip_addr(%.2x, %d) ; is not ACK\n", chip_addr, hkj_WRITE);
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }

    //i2c_stop();
    // restart transmite
    //i2c_begin();

    // send message to mm_i2c device to transmite data
    i2c_write(sub_addr) ;
    if (i2c_read_ack () == NACK)
    {
        i2c_stop();
        i2c_free();
        lidbg("at read funtion :i2c_write (%.2x) is not ACK\n", sub_addr & 0xff);
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }
    i2c_stop();

    // start transmite
    i2c_begin();
    i2c_write_chip_addr(chip_addr, hkj_READ) ;
    if (i2c_read_ack() == NACK)
    {
        i2c_stop();
        i2c_free();
        lidbg("at read funtion :i2c_write_chip_addr(%.2x, %d) ; is not ACK\n", sub_addr, hkj_READ);
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }
    // transmite data
    for(i = 0; i < size; i++)
    {
        buf[i] = i2c_read();
        ( i == (size - 1) ) ? i2c_write_ask(NACK) : i2c_write_ask(ACK);
    }

    // stop transmite
    i2c_stop();
    i2c_free();
    mutex_unlock(&io_i2c_lock);
    return ACK;
#endif
}
i2c_ack i2c_write_byte(int bus_id, char chip_addr, char *buf, unsigned int size)
{
#ifdef FLY_VIDEO_BOARD_V3
    int ret_send = 0;
    ret_send = SOC_I2C_Send(3,  chip_addr, buf, size);
    // lidbg("\n************i2c_write_byte =%d*******\n",ret_send);
    if(ret_send > 0)
    {
#ifdef  DEBUG_ACK
        return NACK;
#else
        return ACK;
#endif
    }
    else
    {
        return NACK;
    }
#else
    u8 i;
    mutex_lock(&io_i2c_lock);
    //lidbg("i2c:write byte:addr =0x%.2x value=0x%.2x ",buf[0],buf[1]);
    i2c_init();

    // start transmite
    i2c_begin();

    i2c_write_chip_addr(chip_addr, hkj_WRITE) ;
    if (i2c_read_ack() == NACK)
    {
        i2c_stop();

        i2c_free();
        lidbg(" at write funtion: i2c_write_chip_addr(%.2x, %d) ; is not ACK\n", chip_addr, hkj_WRITE);
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }


    //i2c_stop();
    // restart transmite
    //i2c_begin();

    // send message to mm_i2c device to transmite data
    i2c_write(buf[0]) ;
    if (i2c_read_ack() == NACK)
    {
        i2c_stop();

        i2c_free();
        lidbg(" at write funtion:i2c_write(%.2x) ;is not ACK\n", buf[0]);
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }
    // transmite data
    for(i = 1; i < size; i++)
    {
        i2c_write(buf[i]);
        if( i2c_read_ack()  == NACK)
        {
            i2c_stop();
            i2c_free();
            lidbg(" at write funtion:i2c_write(%.2x) ;is not ACK\n", buf[i]);
            mutex_unlock(&io_i2c_lock);
            return NACK;
        }
    }

    // stop transmite
    i2c_stop();

    i2c_free();
    mutex_unlock(&io_i2c_lock);
    return ACK;
#endif
}


i2c_ack i2c_read_2byte(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
#ifdef FLY_VIDEO_BOARD_V3
    int ret_rec = 0;
    ret_rec = SOC_I2C_Rec_2B_SubAddr(3, TC358746_I2C_ChipAdd, sub_addr, buf, size);
    //lidbg("\n************i2c_read_2byte  ret_rec=%d *****************\n",ret_rec);
    if(ret_rec > 0)
    {
        return ACK;
    }
    else
    {
        return NACK;
    }
#else
    u8 i;
    mutex_lock(&io_i2c_lock);
    i2c_init();
    // start transmite
    i2c_begin();//msg 0>>
    i2c_write_chip_addr(chip_addr, hkj_WRITE) ;
    if (i2c_read_ack () == NACK)
    {
        i2c_stop();
        i2c_free();
        lidbg(" chip_addr devices is not ACK------i2c_write_chip_addr----r--\r\n");
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }

    //i2c_stop();
    // restart transmite


    // send message to mm_i2c device to transmite data
    i2c_write(sub_addr >> 8) ;
    if (i2c_read_ack () == NACK)
    {
        i2c_stop();
        i2c_free();
        lidbg(" chip_addr devices is not ACK-----subadder1-------\r\n");
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }
    i2c_write(sub_addr) ;
    if (i2c_read_ack () == NACK)
    {
        i2c_stop();
        i2c_free();
        lidbg(" chip_addr devices is not ACK------subadder0------\r\n");
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }
    i2c_stop();//msg 0<<

    // start transmite
    i2c_begin();//msg 1>>
    i2c_write_chip_addr(chip_addr, hkj_READ) ;
    if (i2c_read_ack () == NACK)
    {
        i2c_stop();

        i2c_free();
        lidbg(" chip_addr devices is not ACK----- i2c_write_chip_addr-------\r\n");
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }
    // transmite data
    for(i = 0; i < size; i++)
    {
        buf[i] = i2c_read();
        ( i == (size - 1) ) ? i2c_write_ask(NACK) : i2c_write_ask(ACK);
    }

    // stop transmite
    i2c_stop();//msg 1<<

    i2c_free();
    mutex_unlock(&io_i2c_lock);
    return ACK;
#endif
}

i2c_ack i2c_write_2byte(int bus_id, char chip_addr, char *buf, unsigned int size)
{
#ifdef FLY_VIDEO_BOARD_V3
    int ret_send = 0;
    ret_send = SOC_I2C_Send(3, chip_addr, buf, size);
    //lidbg("\n*************i2c_write_2byt ret_send=%d**************\n",ret_send);
    if(ret_send > 0)
    {
        return ACK;
    }
    else
    {
        return NACK;
    }
#else
    u8 i;
    mutex_lock(&io_i2c_lock);
    i2c_init();

    // start transmite
    i2c_begin();//msg 0>>

    i2c_write_chip_addr(chip_addr, hkj_WRITE) ;
    if (i2c_read_ack() == NACK)
    {
        i2c_stop();

        i2c_free();
        lidbg(" chip_addr devices is not ACK--in-i2c_write_chip_addr(%.2x)----w-----\r\n", chip_addr);
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }

    // send message to mm_i2c device to transmite data
    i2c_write(buf[0]) ;//subaddr MSB
    if (i2c_read_ack () == NACK)
    {
        i2c_stop();
        i2c_free();
        lidbg(" chip_addr devices is not ACK--in-i2c_write(sub[%d])=%.2x-----w----\r\n", 0, buf[0]);
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }
    i2c_write(buf[1]) ;//subaddr
    if (i2c_read_ack () == NACK)
    {
        i2c_stop();
        i2c_free();
        lidbg("chip_addr devices is not ACK--in-i2c_write(sub[%d])=%.2x--w", 1, buf[1]);
        mutex_unlock(&io_i2c_lock);
        return NACK;
    }

    // transmite data
    for(i = 2; i < size; i++)
    {
        i2c_write(buf[i]);
        if( i2c_read_ack()  == NACK)
        {
            i2c_stop();
            i2c_free();
            lidbg(" chip_addr devices is not ACK--in-i2c_write(buf[%d])=%.2x--w----\r\n", (i - 2), buf[i]);
            mutex_unlock(&io_i2c_lock);
            return NACK;
        }
    }

    // stop transmite
    i2c_stop();//msg 0<<

    i2c_free();
    mutex_unlock(&io_i2c_lock);
    return ACK;
#endif
}
#endif //end I2C_IO_IMITATION

/* end of file */
