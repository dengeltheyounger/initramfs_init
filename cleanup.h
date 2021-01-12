#ifndef CLEANUP_H
#define CLEANUP_H
#include <lvm2app.h>
#include <libcryptsetup.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include "logger.h"

void cleanup();

void write_and_die(char *msg, ...);

#endif
