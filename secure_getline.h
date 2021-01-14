#ifndef	GETLINE_PASSWD_H
#define	GETLINE_PASSWD_H
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

/* Get a line from stdin.
 * This is a utility function that is used in get_password
 */

int secure_getline(char **lineptr);

#endif
