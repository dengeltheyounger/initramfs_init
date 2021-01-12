#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <magic.h>
#include <stdlib.h>
#include "utilities.h"
#include "logger.h"
#include "cleanup.h"

#ifndef	TMPFS_MAGIC
#define	TMPFS_MAGIC	0x01021994
#endif

#ifndef RAMFS_MAGIC
#define	RAMFS_MAGIC	0x858458f6
#endif

void destroy_file(const char *d, dev_t rootd);

void switch_root(const char *root_path, const char *init_path);
