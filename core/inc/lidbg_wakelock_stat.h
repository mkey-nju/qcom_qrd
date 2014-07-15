#ifndef _LIGDBG_WAKELOCK_LOG__
#define _LIGDBG_WAKELOCK_LOG__

struct wakelock_item
{
    struct list_head tmp_list;
    char *name;
    char *package_name;
    int cunt;
    int cunt_max;
    bool is_count_wakelock;
    int pid;
    int uid;
};

static inline char *lock_type(bool cnt_wakelock)
{
    return cnt_wakelock ? "java" : "kernel";
}
extern struct list_head lidbg_wakelock_list;
void lidbg_wakelock_stat(int argc, char **argv);
void lidbg_show_wakelock(int is_should_kill_apk);
void lidbg_wakelock_register(bool to_lock, const char *name);

#endif
