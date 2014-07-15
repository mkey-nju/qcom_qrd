#ifndef _LIGDBG_MEM__
#define _LIGDBG_MEM__

void lidbg_mem_main(int argc, char **argv);
void write_phy_addr(u32 PhyAddr, u32 WriteMemValue);
void write_phy_addr_bit(u32 offset, u32 num, u32 phy_addr, u32 value);
u32 read_phy_addr(U32 PhyAddr);
u32 read_phy_addr_bit(u32 offset, u32 num, u32 phy_addr);
u32 read_virt_addr(u32 phy_addr);
void write_virt_addr(u32 phy_addr, u32 value);
u32 read_virt_addr_bit(u32 offset, u32 num, u32 phy_addr);
void write_virt_addr_bit(u32 offset, u32 num, u32 phy_addr, u32 value);

#endif

