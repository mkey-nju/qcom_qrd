#ifndef _LIGDBG_KEY__
#define _LIGDBG_KEY__


#define KEY_RELEASED    0
#define KEY_PRESSED      1
#define KEY_PRESSED_RELEASED    2

void lidbg_key_main(int argc, char **argv);
void lidbg_key_report(u32 key_value, u32 type);



#endif

