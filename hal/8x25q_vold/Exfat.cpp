#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>

#include <linux/kdev_t.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include "Exfat.h"

static char FSCK_EXFAT_PATH[] = "/system/bin/fsck.exfat";
static char MK_EXFAT_PATH[] = "/system/bin/mkfs.exfat";


extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Exfat::check(const char *fsPath)
{
    SLOGI("Exfat::check");
    if (access(FSCK_EXFAT_PATH, X_OK))
    {
        SLOGW("Skipping fs checks\n");
        return 0;
    }

    int rc = 0;

    const char *args[5];
    args[0] = FSCK_EXFAT_PATH;
    args[1] = fsPath;
    args[2] = NULL;

    rc = logwrap(2, args, 1);
    if( rc != 0 )
    {
        SLOGE("Filesystem check failed (unknown exit code %d)", rc);
    }

    return rc;
}

