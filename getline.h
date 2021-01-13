#ifndef	GETLINE_H
#define	GETLINE_H
#include <stdlib.h>
#include <stddef.h>

/* Get a line from stdin.
 * This is a utility function that is used in get_password
 */

int getline(char **lineptr);

#endif
