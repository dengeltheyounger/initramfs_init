#include "logger.h"

// This determines whether or not message gets logged
log_level level = 0;

// Open console and set level. Return success or error
void open_logger(log_level l) {
	level = l;
}

void write_procedural(char *msg, ...) {
	va_list args;
	va_start(args, msg);

	vfprintf(stderr, msg, args);

	va_end(args);
}

// Print message if needed
void write_logger(log_level l, char *print_fmt, ...) {

	// Just exit if the log level is greater
	if (level < l) 
		return;

	// Create va list and print
	va_list args;
	va_start(args, print_fmt);

	vfprintf(stderr, print_fmt, args);

	// Clean up and exit
	va_end(args);
}

log_level get_log_level() {
	return level;
}
