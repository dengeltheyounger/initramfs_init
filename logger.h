#ifndef LOGGER_H
#define LOGGER_H
#include <stdio.h>
#include <stdarg.h>

typedef enum log_level {
	critical,
	attention,
	warning,
	debug
} log_level;

void open_logger(log_level);

void write_logger(log_level, char *, ...);

/* This function will always write to console. It is not intended to
 * log critical conditions. Instead, it is intended to print messages that
 * are a normal part of the init process (mainly cryptsetup stuff
 */

/* The reason I have this as returning void is because there really
 * isn't a lot that can be done if the program won't print to console.
 * If it's not printing, all we can really do is just exit and try again
 * some other time
 */
void write_procedural(char *, ...);

log_level get_log_level();

#endif
