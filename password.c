#include "password.h"

// Line will contain the malloc'd buffer

size_t get_password(char **line) {
	struct termios before,after;

	/* Turn echoing off. Fail if it didn't work */

	if (tcgetattr(fileno (stdin), &before) != 0)
		return 0;

	after = before;

	after.c_lflag &= ~(ICANON | ECHO);

	if (tcsetattr (fileno (stdin), TCSAFLUSH, &after) != 0)
		return 0;

	size_t read = secure_getline(line);
		
	// restore terminal
	tcsetattr (fileno (stdin), TCSAFLUSH, &before);

	return read;
}
