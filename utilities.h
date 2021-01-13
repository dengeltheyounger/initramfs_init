#ifndef	UTILITIES_H
#define	UTILITIES_H
#include <dirent.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include "logger.h"

#define MAKE_DEV	1
#define MAKE_PROC 	2
#define MAKE_SYS	4

// Uses or flags to determine what to set up
int set_mounts(int flags);

// Opens the console rw and duplicates file descriptors
int set_console();


// Close the console using the provided file descriptor
int close_console(int fd);

/* Get the size of a file.
 * 0 indicates that an error occurred
 * usually
 */
size_t get_file_size(const char *path);

/* This is a utility function that concatenates two paths together.
 * end is attached to the beginning of end.
 *
 * make_path will dynamically allocate memory for the path
 * make_path_static will rely a buffer that is given
 *
 * slash refers to whether or not add slash to the end of path
 */

int make_path_static(const char *begin, 
			const char *end, 
			char **buffer, 
			size_t buffersize,
			int slash);

char *make_path(const char *begin, const char *end, int slash);

// These are useful utilities that can help with debugging

#ifdef DEBUG

/* Iterate through directories. Provide a callback so that we can
 * provide a generic way to handle different directories. The primary
 * purpose of this is to look through proc in order to figure what 
 * files are open in a given filesystem
 */

int iterate_directories(char *path, 
			char *target, 
			void (*callback)(DIR *, const char *, const char *));

// Print contents of file
int print_cmdline(const char *path);

/* Get all open files under a given process. Print the ones that
 * contain path "/mnt/path"
 */

void get_open_files(DIR *directory, const char *name, const char *target);

int copy(const char *from, const char *to);

#endif

#endif
