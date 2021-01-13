#include "getline.h"

/* Round up a number to the nearest power of 2.
 * This assumes a 64 bit machine.
 * This is used to determine how much to reallocate 
 * for the line
 */
size_t round_nearest_2(size_t v) {

	if (v == 1)
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

/*  1 -> 32-126
 * -1 -> 8 || 127
 *  0 -> everything else
 */
int get_char_type(char c) {
	if (c == 8 || c == 127)
		return -1;
	if (c >= 32 && c < 127)
		return 1;
	
	return 0;
}

/* For a given null terminated buffer, count the number of valid
 * characters and backspaces.
 * Valid characters constitutes all printable characters except for \n
 *
 */

void distinguish_chars(char *buffer, 
			size_t buffer_size, 
			size_t *valid, 
			size_t *backspaces) {
	
	for (size_t i = 0; i < buffer_size; ++i) {
		if (buffer[i] == 8 || buffer[i] == 127)
			++(*backspaces);

		else if (buffer[i] >= 32 && buffer[i] < 127)
			++(*valid);
	}
}

/* This handles memory allocation for getline.
 * It will take the amount written into the line,
 * add the requested extension to that and then
 * determine whether or not the total is greater than
 * the size of the line. If it is not, then it will
 * not realloc. If the total is greater than the 
 * size of the line, then it will round up to the
 * nearest power of 2.
 *
 * 0 indicates that there was a failure in reallocing.
 * 1 indicates that there were no errors encountered
 */

int realloc_line(char **line,
			size_t written,
			size_t requested,
			size_t *size) {
	
	size_t total = written + requested;

	// Don't realloc unless the total written > size of buffer
	if (total < (*size))
		return 1;

	// Round up the total to nearest power of 2
	size_t newsize = round_nearest_2(total);
	// reallocate
	char *tmp = realloc(*line, newsize);

	// exit false realloc failure
	if (!tmp)
		return 0;

	// 0 the newly allocated memory
	memset(tmp + newsize - *size, 0, newsize - *size);

	// Point line to newly allocated memory
	*line = tmp;
	// Update size
	*size = newsize;

	return 1;
}

/* This function remove characters from the line based on the
 * number of backspaces that it is given. It will also update
 * the total number of characters that are written to the line
 */

void backspace_line(char **line,
			size_t *written,
			size_t backspaces) {

	// Apply backspaces. Exit early if written is zero
	for (size_t i = backspaces; i > 0; --i) {
		if ((*written) == 0) {
			break;
		}

		(*line)[--(*written)] = 0;
	}
}
