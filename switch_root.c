#include "switch_root.h"

// Most of this implementation is derived from busybox's switch_root

void destroy_file(const char *d_char, dev_t rootd) {
	DIR *dir;
	struct dirent *d_ent;
	struct stat fs_check;

	// Make sure we're not in a different filesystem
	if (lstat(d_char, &fs_check) || fs_check.st_dev != rootd)
		return;
	
	// If not directory, zap
	if (!S_ISDIR(fs_check.st_mode))
		unlink(d_char);

	// Now we know that d is a directory
	// Open directory
	dir = opendir(d_char);

	// Exit if opendir failed
	if (!dir) 
		return;

	// Recursively delete contents of directories
	while ((d_ent = readdir(dir))) {
		// Get name of directory
		char *newdir = d_ent->d_name;

		// Skip if directory is . or ..
		if (strcmp(newdir, ".") == 0 || strcmp(newdir, "..") == 0)
			continue;

		// Create new path for recursion
		newdir = make_path(d_char, newdir, 1);

		// recurse
		destroy_file(newdir, rootd);
		// free newly allocated memory
		free(newdir);
	}

	// Close directory
	closedir(dir);
	
	// Delete directory
	rmdir(d_char);
}

// If something happens, make emergency dump of information and then poweroff
void emergency_dump(char *msg) {
	set_console();
	write_and_die(msg);
}

void switch_root(const char *root_path, const char *init_path) {
	struct stat st;
	struct statfs stfs;
	dev_t rootd;
	int result = 0;
	int fd;

	// Open the logger in case an emergency dump is necessary
	open_logger(critical);

	// Change to real root directory, and verify different fs
	if (chdir(root_path) < 0) {
		emergency_dump("Failed to chdir to real root!\n");
	}

	if (stat("/", &st) < 0) {
		emergency_dump("Failed to stat fake root!\n");
	}

	rootd = st.st_dev;

	if (stat(".", &st) < 0) {
		emergency_dump("Failed to stat real root!\n");
	}

	if (st.st_dev == rootd) {
		emergency_dump("New root must be a mountpoint!\n");
	}

	// Perform busybox's sanity check
	
	if (stat("/init", &st) != 0 || !S_ISREG(st.st_mode)) {
		emergency_dump("/init is not a regular file!\n");
	}

	statfs("/", &stfs);

	if (stfs.f_type != RAMFS_MAGIC && stfs.f_type != TMPFS_MAGIC) {
		emergency_dump("Fake root filesystem is not ramfs/tmpfs!\n");
	}

	// Begin process of deleting rootdev
	destroy_file("/", rootd);

	// Mount root
	if (mount(".", "/", NULL, MS_MOVE, NULL) < 0) {
		emergency_dump("Failed to move real root onto fake root!\n");
	}

	// Chroot
	if (chroot(".") < 0) {
		emergency_dump("Chroot failed to real root failed!\n");
	} 

	if (chdir("/") < 0) {
		emergency_dump("Chdir to / failed!\n");
	}

	set_console();

	// Execute init
	execl(init_path, strrchr(init_path, '/'), NULL);

	write_and_die("Failed to execute init!\n");
}
