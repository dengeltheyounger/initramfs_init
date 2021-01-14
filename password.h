#ifndef PASSWORD_H
#define PASSWORD_H
#include <termios.h>
#include "secure_getline.h"

/* Get password from user.
 * Return number of characters.
 * Will read from stdin
 */

size_t get_password(char **line);

#endif
