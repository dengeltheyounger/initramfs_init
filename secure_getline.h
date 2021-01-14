#ifndef SECURE_GETLINE_H
#define	SECURE_GETILNE_H
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

/* Get a line from stdin.
 * This is a utility function that is used in get_password
 */

size_t secure_getline(char **lineptr);

#endif
