#include "secure_getline.h"

/* Round up a number to the nearest power of 2.
 * This assumes a 64 bit machine.
 * This is used to determine how much to reallocate 
 * for the line
 */
size_t round_nearest_2(size_t v) {

	if (v <= 1)
		return 2;

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v++;

	return v;
}

/* Clear previous buffer before freeing. 
 * return 1 for successful allocation
 * return 0 for allocation error
 */
int secure_realloc(char **buffer, size_t old_size, size_t new_size) {

	// Allocate new memory
	char *tmp = calloc(1, new_size);

	if (!tmp)
		return 0;

	// If the buffers empty, point it to tmp and exit
	if (!*buffer) {
		*buffer = tmp;
		return 1;
	}

	// Copy old memory
	tmp = memcpy(tmp, *buffer, old_size);
	
	// Clear old memory
	memset(*buffer, 0, old_size);

	// Free old memory and point to new
	free(*buffer);

	*buffer = tmp;
	
	// Exit success
	return 1;
}

size_t secure_getline(char **lineptr) {
	*lineptr = NULL;
	int char_num = 0;
	int success = 0;
	int c = 0;
	// We're going to start with a 256 byte buffer
	size_t old_size = 256;
	size_t index = 0;
	size_t new_size = 256;

	success = secure_realloc(lineptr, old_size, new_size);
	
	// Exit failure
	if (!success)
		return 0;

	/* Keep getting input until we hit an error, newline, or EOF.
	 * Store in buffer and account for backspaces
	 */
	while ((success = read(STDIN_FILENO, &c, 1)) >= 0) {
		if (c == '\n' || c == EOF)
			break;
		// Handle backspaces
		else if (c == 8 || c == 127) {
			if (index > 0)
				(*lineptr)[--index] = 0;
			else
				(*lineptr)[index] = 0;
				
		}

		else if (c >= 32 && c < 127)
			(*lineptr)[index++] = c;

		// If we have reached the end of our buffer, realloc
		if (index == old_size) {
			// Round up size to next power of 2
			new_size = round_nearest_2(++old_size);	
			success = secure_realloc(lineptr, old_size, new_size);
			if (!success)
				return 0;
		}

		c = 0;
	}
	
	// Return the total number of characters written
	return index + 1;
}

