#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "password.h"

int main() {
	char *lineptr = NULL;

	int written = get_password(&lineptr);

	if (written < 0) {
		fprintf(stderr, 
			"There was an error during first iteration!\n");
		return 1;
	}

	fprintf(stderr, "Line 1: %s\n", lineptr);

	memset(lineptr, 0, written);

	free(lineptr);

	written = get_password(&lineptr);

	if (written < 0) {
		fprintf(stderr, 
			"There was an error during second iteration!\n");
		return 1;
	}

	fprintf(stderr, "Line 2: %s\n", lineptr);

	memset(lineptr, 0, written);

	free(lineptr);

	return 0;
}
