
#ifndef _LIGDBG_TRACE_MSG__
#define _LIGDBG_TRACE_MSG__

#define LIDBG_TRACE_MSG_PATH LIDBG_LOG_DIR"lidbg_trace_msg.txt"

void lidbg_trace_msg_disable(int flag);
void trace_msg_main(int argc, char **argv);
bool lidbg_trace_msg_cb_register(char *key_word, void *data, void (*cb_func)(char *key_word, void *data));
#endif
