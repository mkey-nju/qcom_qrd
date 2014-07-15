

#include "lidbg.h"

void lidbg_mem_main(int argc, char **argv)
{
    if(argc < 3)
    {
        lidbg("Usage:\n");
        lidbg("r phyaddr num \n");
        lidbg("w phyaddr value \n");

        lidbg("rb phyaddr offset bitnum \n");
        lidbg("wb phyaddr offset bitnum value \n");
        return;

    }

    if(!strcmp(argv[0], "r"))
    {

        u32 addr, val = 0, mun;
        //lidbg("r addr num \n");

        //addr = simple_strtoul(argv[1], 0, 0);
        //mun = simple_strtoul(argv[2], 0, 0);
        addr = simple_strtoul(argv[2], 0, 0);
        mun = simple_strtoul(argv[3], 0, 0);
        while(mun)
        {
            if(!strcmp(argv[1], "phy"))
                val = read_phy_addr(addr);
            else if(!strcmp(argv[1], "virt"))
                val = read_virt_addr(addr);

            lidbg("read %s addr=0x%x, value=0x%x\n", argv[1], addr, val);
            addr += 4;
            mun--;
        }
    }
    else if(!strcmp(argv[0], "w"))
    {
        u32 addr, val;

        addr = simple_strtoul(argv[2], 0, 0);
        val = simple_strtoul(argv[3], 0, 0);

        if(!strcmp(argv[1], "phy"))
        {
            write_phy_addr(addr, val);
            //read back
            val = read_phy_addr(addr);
            lidbg("w_phy_addr=0x%x, read back value=0x%x\n", addr, val);
        }
        else if(!strcmp(argv[1], "virt"))
        {
            write_virt_addr(addr, val);
            //read back
            val = read_virt_addr(addr);
            lidbg("w_virt_addr=0x%x, read back value=0x%x\n", addr, val);
        }

    }

    else if(!strcmp(argv[0], "wb"))
    {
        u32 addr, val, offset, bitmun;
        addr = simple_strtoul(argv[2], 0, 0);
        offset = simple_strtoul(argv[3], 0, 0);
        bitmun = simple_strtoul(argv[4], 0, 0);
        val = simple_strtoul(argv[5], 0, 0);

        if(!strcmp(argv[1], "phy"))
        {
            write_phy_addr_bit(offset, bitmun, addr, val);
            //read back
            val = read_phy_addr_bit(offset, bitmun, addr);
            lidbg("wb_phy_addr=0x%x, read back value=0x%x\n", addr, val);
        }
        else if(!strcmp(argv[1], "virt"))
        {
            write_virt_addr_bit(offset, bitmun, addr, val);
            //read back
            val = read_virt_addr_bit(offset, bitmun, addr);
            lidbg("wb_virt_addr=0x%x, read back value=0x%x\n", addr, val);
        }
    }
    else if(!strcmp(argv[0], "rb"))
    {

        u32 addr, val = 0, offset, bitmun;
        addr = simple_strtoul(argv[2], 0, 0);
        offset = simple_strtoul(argv[3], 0, 0);
        bitmun = simple_strtoul(argv[4], 0, 0);

        if(!strcmp(argv[1], "phy"))
            val = read_phy_addr_bit(offset, bitmun, addr);
        else if(!strcmp(argv[1], "virt"))
            val = read_virt_addr_bit(offset, bitmun, addr);

        lidbg("read bit %s addr=0x%x, value=0x%x\n", argv[1], addr, val);
    }

}

void write_phy_addr_bit(u32 offset, u32 num, u32 phy_addr, u32 value)
{

    u32  Mark, i, Temp;

    Mark = 0x01;
    for(i = 0; i < num - 1; i++)
        Mark = (Mark << 1) | 0x1;

    Temp = read_phy_addr(phy_addr);

    Temp = (Temp & (~(Mark << offset))) | ((value & Mark) << offset);

    write_phy_addr(phy_addr, Temp);

}

void write_virt_addr_bit(u32 offset, u32 num, u32 phy_addr, u32 value)
{

    u32  Mark, i, Temp;

    Mark = 0x01;
    for(i = 0; i < num - 1; i++)
        Mark = (Mark << 1) | 0x1;

    Temp = read_virt_addr(phy_addr);

    Temp = (Temp & (~(Mark << offset))) | ((value & Mark) << offset);

    write_virt_addr(phy_addr, Temp);

}

void write_phy_addr(u32 phy_addr, u32 value)
{

    u32 *p_address;
    //lidbg("write_phy_addr :phy_addr 0x%x, value 0x%x\n", phy_addr, value);

#if 1
    p_address = ioremap(phy_addr, 4);
    *p_address = value;
    iounmap(p_address);
#else
    *(u32 *)(IO_PA2VA_AHB(phy_addr)) = value;
#endif
    //__raw_writel(value, phys_to_virt(phy_addr));

}

void write_virt_addr(u32 phy_addr, u32 value)
{

    *(U32 *)phy_addr = value;

}

u32 read_phy_addr_bit(u32 offset, u32 num, u32 phy_addr)
{
    u32  Mark, i, Value;

    Mark = 0x01;
    for(i = 0; i < num - 1; i++)
        Mark = (Mark << 1) | 0x1;

    Value = read_phy_addr(phy_addr);
    Value = (Value & (Mark << offset)) >> offset;
    return Value;

}

u32 read_virt_addr_bit(u32 offset, u32 num, u32 phy_addr)
{
    u32  Mark, i, Value;

    Mark = 0x01;
    for(i = 0; i < num - 1; i++)
        Mark = (Mark << 1) | 0x1;

    Value = read_virt_addr(phy_addr);
    Value = (Value & (Mark << offset)) >> offset;
    return Value;

}

u32 read_phy_addr(u32 phy_addr)

{
    u32 *p_address;
    u32 val;
    p_address = ioremap(phy_addr, 4);
    val = *p_address;
    iounmap(p_address);
    return val;
}

u32 read_virt_addr(u32 phy_addr)
{
    return *(U32 *)phy_addr;

}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");


EXPORT_SYMBOL(read_virt_addr);
EXPORT_SYMBOL(write_virt_addr);
EXPORT_SYMBOL(read_phy_addr);
EXPORT_SYMBOL(write_phy_addr);
EXPORT_SYMBOL(write_phy_addr_bit);
EXPORT_SYMBOL(read_phy_addr_bit);
EXPORT_SYMBOL(write_virt_addr_bit);
EXPORT_SYMBOL(read_virt_addr_bit);
EXPORT_SYMBOL(lidbg_mem_main);

