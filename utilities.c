#include "utilities.h"

int set_mounts(int flags) {
	int result = 0;

	// Mount depending on the flags
	switch (flags) {
		case 7:
			if (mount("none", "/dev", "devtmpfs", 0, "") < 0)
				return 7;
			if (mount("none", "/proc", "proc", 0, "") < 0)
				return 7;
			if (mount("none", "/sys", "sysfs", 0, "") < 0)
				return 7;
			break;
		case 6:
			if (mount("none", "/proc", "proc", 0, "") < 0)
				return 6;
			if (mount("none", "/sys", "sysfs", 0, "") < 0)
				return 6;
			break;
		case 5:
			if (mount("none", "/dev", "devtmpfs", 0, "") < 0)
				return 5;
			if (mount("none", "/sys", "sysfs", 0, "") < 0)
				return 5;
			break;
		case 4:
			if (mount("none", "/sys", "sysfs", 0, "") < 0)
				return 4;
			break;
		case 3: if (mount("none", "/dev", "devtmpfs", 0, "") < 0)
				return 3;
			if (mount("none", "/proc", "proc", 0, "") < 0)
				return 3;
			break;
		case 2: if (mount("none", "/proc", "proc", 0, "") < 0)
				return 2;
			break;
		case 1: if (mount("none", "/dev", "devtmpfs", 0, "") < 0)
				return 1;
			break;
		default: return 8;
	}

	return 0;
}

int set_console() {
	// Open /dev/console rw
	int fd = open("/dev/console", O_RDWR);

	if (fd < 0)
		return -1;

	// file descriptors are now associated with /dev/console
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);

	// Not sure why this is being done. klibc did it this way
	if (fd > STDERR_FILENO) {
		return close(fd);
	}

	// Success
	return fd;
}

int close_console(int fd) {
	return close(fd);
}

// Get the size of the given file
size_t get_file_size(const char *path) {
	FILE *fp = fopen(path, "r");
	size_t size = 0;

	if (!fp) {
		write_logger(debug, "Failed to open file %s\n", path);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	fclose(fp);

	return size;
}

int make_path_static(const char *begin,
			const char *end,
			char **buffer,
			size_t buffersize,
			int slash) {
	// If 1, then put a slash at the end of begin
	int begin_slash = 0;
	size_t bsize = 0;
	size_t esize = 0;
	char *bend = NULL;
	
	// Get the size of begin and end
	bsize = strlen(begin);
	esize = strlen(end);
	// zero out the buffer
	memset(*buffer, 0, buffersize);

	// Make sure that buffer is big enough to contain path

	if (begin[bsize - 1] == '/') {
		if (buffersize < (bsize+esize+1))
			return 0;
	}
	else {
		if (buffersize < (bsize+esize+2))
			return 0;
		// indicate that a slash needs to be added to the end of begin
		++begin_slash;
	}

	// Copy begin into buffer
	*buffer = strcpy(*buffer, begin);

	if (begin_slash) {
		bend = *buffer + strlen(*buffer);
		*bend++ = '/';
		strncpy(bend, end, esize);
	
		if (slash) 
			*(*buffer+strlen(*buffer)) = '/';

		return 1;
	}

	// Since begin_slash == 0, proceed as normal
	
	bend = *buffer + strlen(*buffer);
	strncpy(bend, end, esize);

	if (slash)
		*(*buffer+strlen(*buffer)) = '/';

	// final should contain a concatenation of both begin and end
	return 1;
		
}

char *make_path(const char *begin, const char *end, int slash) {

	size_t buffersize = 0;
	size_t bsize = strlen(begin);
	size_t esize = strlen(end);
	char *final = NULL;
	// Get new string of length both strings + 1
	/* We also want to make sure that begin has a
	 * slash at the end. Otherwise, we're going to
	 * allocate an extra byte to accomodate
	 */
	if (begin[bsize - 1] == '/') {
		buffersize = bsize+esize+1;
		final = calloc(1, buffersize);
	}
	else {
		buffersize = bsize+esize+2;
		final = calloc(1, buffersize);
	}

	if (!final) {
		write_logger(attention,
			"Failed to allocate memory for make_path!\n");	
		return NULL;
	}
	
	// Call make_path_static
	int result = make_path_static(begin, end, &final, buffersize, slash);
	// return final or NULL depending on success or error
	return (result) ? final : NULL;
}


#ifdef DEBUG

int iterate_directories(char *path, 
			char *target, 
			void (*callback)(DIR *, const char *, const char *)) {
	struct dirent *entity = NULL;
	DIR *d = NULL;
	DIR *d_recurse = NULL;
	struct stat st;
	// path of file in directory
	char *fpath = NULL;

	// Exit failure if we can't open directory
	if ((d = opendir(path)) == NULL) {
		write_logger(attention,
				"Failed to open directory %s!\n",
				path);
		return 0;
	}

	while ((entity = readdir(d)) != NULL) {
		// Change working directory to path
		chdir(path);
		if (strcmp(entity->d_name, ".") == 0 ||
			strcmp(entity->d_name, "..") == 0)
			continue;

		fpath = make_path(path, entity->d_name, 0);

		if (stat(fpath, &st) < 0) {
			write_logger(attention,
					"Failed to stat file %s!\n",
					fpath);
			free(fpath);
			continue;
		}

		// Check if directory
		// If so, give to callback
		if (S_ISDIR(st.st_mode)) {
			d_recurse = opendir(entity->d_name);
			
			if (!d_recurse) {
				write_logger(attention,
					"Failed to open directory %s!\n",
					entity->d_name);
				free(fpath);
				continue;
			}

			callback(d_recurse, entity->d_name, target);
			// Close when done
			closedir(d_recurse);
		}
		
		// Free the allocated path
		free(fpath);
	}

	closedir(d);
}

// This is mainly to be used for reading /proc stuff
void strip_null(char *buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		if (buffer[i] == '\0') {
			buffer[i] == ' ';
		}
	}
}

int print_cmdline(const char *path) {
	// Opening read only
	size_t buffersize = get_file_size(path);
	size_t read = 0;

	// if buffersize is zero, attempt secondary count
	if (buffersize == 0)
		buffersize = 512;

	// set up buffer
	char buffer[buffersize+1];
	memset(buffer, 0, buffersize+1);

	FILE *fp = fopen(path, "r");

	if (!fp) {
		write_logger(attention,
			"Error opening file for reading\n");
		return 0;
	}

	read = fread(buffer, 1, buffersize, fp);
	// Keep reading until the total read is less than buffersize
	do {
		strip_null(buffer, read);
		// The end of buffer should be null terminated
		write_procedural("%s", buffer);
		read = fread(buffer, 1, buffersize, fp);
		/* Clear everything after what was read in
		 * as long as something is being read in
		 */
		if (read < buffersize && read != 0)
			memset(buffer+read, 0, buffersize - read);
	} while (read == buffersize);

	fclose(fp);
	// One more time around if there's anything left
	if (read != 0)
		write_procedural("%s", buffer);
	return 1;
}
	
/* Call atoi. It'll return 0 if there was an error in converting.
 * It should succeed if all characters are numeric.
 */
int is_numeric(const char *name) {
	return (atoi(name) != 0);
}

void get_open_files(DIR *directory, const char *name, const char *target) {
	// Exit if the name is not either a sequence of numbers or a directory
	if (!is_numeric(name))
		return;
	
	if (!directory)
		return;

	struct dirent *entity = NULL;
	DIR *fd = NULL;
	// path of dereferenced symbolic link
	char *openreal = NULL;
	// path of symbolic link
	char *openpath = NULL;
	// Base path that contains the symbolic links
	char *basepath = make_path("/proc", name, 0);
	char *fdpath = NULL;
	char *cmdline = make_path(basepath, "cmdline", 0);
	chdir(basepath);

	// Attempt to open the fd directory in the process directory
	fd = opendir("fd");

	if (!fd) {
		write_logger(attention,
				"Failed to open fd in %s!\n", name);
		return;
	}

	fdpath = make_path(basepath, "fd", 0);
	free(basepath);
	// Working directory is /proc/[0-9]+/fd
	chdir(fdpath);
	free(fdpath);

	while ((entity = readdir(fd)) != NULL) {
		// Skip cases of . or ..
		if (strcmp(entity->d_name, ".") == 0 || 
			strcmp(entity->d_name, "..") == 0)
			continue;
	
		// malloc will take care of this
		if ((openreal = realpath(entity->d_name, NULL)) == NULL) {
			free(openpath); free(openreal);
			continue;
		}
		// if openreal contains /mnt/crypt, print
		if (strstr(openreal, target)) {

			//write_procedural("Name: ");
			write_logger(debug, "Name: ");
			print_cmdline(cmdline);
			//write_procedural("\n%s\n", openreal);
			write_logger(debug, "\n%s\n", openreal);
		}

		// Reset things for the next symbolic link
		free(openpath); free(openreal);
	}

	free(cmdline);
	closedir(fd);
	return;
}

int copy(const char *from, const char *to) {
	FILE *f = NULL;
	FILE *t = NULL;
	size_t fsize = 0;
	size_t readf, writet;
	char *buffer = NULL;

	fsize = get_file_size(from);

	if (!fsize)
		return 0;
	
	// Open from file in read mode
	f = fopen(from, "r");

	if (!f)
		return 0;
	
	// Open to file in write mode
	t = fopen(to, "w");

	if (!t) {
		fclose(f);
		return 0;
	}

	// allocate enough memory to contain file
	buffer = malloc(sizeof(char)*fsize);

	if (!buffer) {
		fclose(f);
		fclose(t);
		return 0;
	}

	
	// read from into buffer
	readf = fread(buffer, 1, fsize, f);
	// no need to have f open anymore
	fclose(f);

	if (readf < fsize) {
		free(buffer);
		fclose(t);
		return 0;
	}

	// write out t
	writet = fwrite(buffer, 1, fsize, t);

	// Free memory
	free(buffer);
	fclose(t);
	// Report whether or not copy was successful
	return (writet == fsize);
}

#endif
