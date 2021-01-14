#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <libcryptsetup.h>
#include <lvm2app.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "switch_root.h"
#include "logger.h"
#include "cleanup.h"
#include "password.h"
#include "utilities.h"

/* This function will decrypt a given device. It is assumed that the
 * device is unlocked by passphrase.
 *
 * Path refers to the path of the device file.
 * Name refers to the name that is to be given to the unlocked device
 */
struct crypt_device *decrypt_device(const char *path, const char *name) {
	struct crypt_device *handle = NULL;
	int result = 0;

	// Initialize crypt device handle
	result = crypt_init(&handle, path);

	if (result < 0) {
		write_logger(critical, 
			"Failed to initialize crypt device handler for %s!\n",
			name);
		return NULL;
	}

	// Load header into crypt_device context
	result = crypt_load(handle, CRYPT_LUKS2, NULL);
	
	if (result < 0) {
		write_logger(critical, 
				"Failed to load header for device %s!\n",
				name);
		crypt_free(handle);
		return NULL;
	}

	result = -1;

	for (int i = 0; result < 0 && i < 3; ++i) {
		char *buffer = NULL;
		size_t charnum = 0;
		write_procedural("Please enter the passphrase for %s:\n", path);
		charnum = get_password(&buffer);

		// If get_password fails, then silently go to next loop
		if (charnum == 0) {
			memset(buffer, 0, strlen(buffer));
			continue;
		}

		// Open encrypted drives read only
		result = crypt_activate_by_passphrase(handle,
						      name,
						      CRYPT_ANY_SLOT,
						      buffer, strlen(buffer),
						      0);
		// Zero the buffer. Leave no record of passphrase.
		memset(buffer, 0, strlen(buffer));
		free(buffer); buffer = NULL;
	}

	if (result < 0) {
		write_logger(critical, 
				"Failed to unlock passphrase for %s!\n",
				path);

		crypt_free(handle);
		return NULL;
	}

	return handle;
}

/* This function will decrypt the drive on which the root filesystem resides.
 * It is assumed that the header is detached and that a keyfile is used to 
 * unlock. 
 * In addition, it assumes that the path to the header is found on
 * /mnt/crypt/header.img
 * It also assumes that the path to the keyfile is on /mnt/key.
 *
 * root_path refers to the path of the device file that represents the
 * encrypted drive.
 *
 * key_name refers to the name of the keyfile.
 */

int decrypt_detached(const char *root_path,
			const char *key_name) {

	struct crypt_device *drive = NULL;
	// Create buffer and size for key path
	size_t size = strlen(key_name)+strlen("/mnt/key")+2;
	char buffer[size];
	char *bptr = buffer;

	// Create new path to key file
	int result = make_path_static("/mnt/key", 
				key_name, 
				&bptr, 
				size, 0);

	if (!result) {
		write_logger(attention, "Failed to create path for %s!\n", 
				key_name);
		return 0;
	}

	// initialize root drive specifying path to header
	result = crypt_init_data_device(&drive, "/mnt/crypt/header.img", 
							root_path);

	if (result < 0 || !drive) {
		write_logger(critical, "Failed to initialize cryptroot!\n");
		return 0;
	}

	// load header into context
	result = crypt_load(drive, CRYPT_LUKS2, NULL);

	if (result < 0) {
		write_logger(critical, "Failed to load cryptroot header!\n");
		crypt_free(drive);
		return 0;
	}

	result = crypt_activate_by_keyfile(drive, 
					"cryptroot",
					CRYPT_ANY_SLOT, 
					buffer,
					get_file_size("/mnt/key/key"), 
					0);
	if (result < 0) {
		write_logger(critical, "Failed to open root device!\n");
		crypt_free(drive);
		return 0;
	}

	crypt_free(drive);

	return 1;
}

/* Open drive will open a hard drive that is encrypted. Open drive
 * assumes that there is an encrypted boot, an encrypted keyfile inside
 * of the encrypted boot, and that the encrypted hard drive has a detached
 * luks header that is also contained inside of the encrypted boot.
 *
 * root_path refers to the path to the device file that represents the
 * encrypted drive that contains the root filesystem
 *
 * boot_path refers to the path to the device file that represents the
 * encrypted boot partition
 *
 * key_name refers to the name of the keyfile contained in key.img
 */

int open_drive(const char *root_path, 
		const char *boot_path,
		const char *crypt_key_name,
		const char *key_name) {

	int result = 0;
	// Open the first device
	struct crypt_device *first = decrypt_device(boot_path, "cryptboot");

	if (!first) {
		return 0;
	}

	// Mounting read only because nothing needs to be written to
	if (mount("/dev/mapper/cryptboot", 
			"/mnt/crypt", 
			"ext4", 
			0, NULL) < 0) {
		write_logger(critical, "Failed to mount cryptboot!\n");
		crypt_free(first);
		return 0;
	}

	// Create a path for the encrypted keyfile
	size_t key_path_size = strlen("/mnt/crypt/");
	size_t crypt_name_size = strlen(crypt_key_name);
	size_t total_size = key_path_size + crypt_name_size + 1;
	char crypt_key_path[total_size];
	char *crypt_ptr = crypt_key_path;
	memset(crypt_key_path, 0, total_size);
	result = make_path_static("/mnt/crypt", 
				crypt_key_name, 
				&crypt_ptr,
				total_size,
				0);

	if (!result) {
		write_logger(attention, "Failed to create path for %s!\n", 
				crypt_key_name);
		crypt_free(first);
		return 0;
	}
	struct crypt_device *second = decrypt_device(crypt_key_path, 
							"cryptkey");

	if (!second) {
		crypt_free(first);
		return 0;
	}

	// Attempt to mount it
	if (mount("/dev/mapper/cryptkey", 
			"/mnt/key", 
			"ext4", 
			0, NULL) < 0) {
		write_logger(critical, "Failed to mount cryptkey!\n");
		crypt_free(second);
		crypt_free(first);
		return 0;
	}

	// detached_header explains that something went wrong
	if (!decrypt_detached(root_path, key_name)) {
		crypt_free(second);
		crypt_free(first);
		return 0;
	}

	// Unmount and close devices since root has now been decrypted
	if (umount("/mnt/key") < 0) {
		write_logger(attention, 
				"Umount error on cryptkey!\n");
	}
	
	// We're going to try a forceful deactivate
	if (crypt_deactivate(second, "cryptkey") < 0) {
		write_logger(attention, "Failed to deactivate cryptkey!\n");
	}

	crypt_free(second);
	
	if (umount("/mnt/crypt") < 0) {
		write_logger(attention,
				"Umount error on ryptboot!\n");
	}

	if (crypt_deactivate(first,"cryptboot") < 0) {
		write_logger(attention, "Failed to deactivate cryptboot!\n");
	}
	
	crypt_free(first);

	return 1;
}

// For a given volume group, activate the provided set of logical volumes 

int set_lvm(const char *gname, ...) {
	// initialize variadic arguments
	va_list vols;
	va_start(vols, gname);
	const char *volume = va_arg(vols, const char *);
	// Create handler
	lvm_t libh = lvm_init(NULL);
	lv_t sys;
	vg_t vg;

	// Any of these errors are a deal breaker, so we'll exit in each case
	if (!libh) {
		write_logger(critical, "Failed to create lvm handler!\n");
		va_end(vols);
		return 0;
	}

	// Scan the volumes
	lvm_scan(libh);
	// Open SYSTEM volume group
	vg = lvm_vg_open(libh, gname, "w", 0);

	if (!vg) {
		write_logger(critical, "Failed to create vg handler!\n");
		lvm_quit(libh);
		va_end(vols);
		return 0;
	}

	// Activate each volume
	while (volume != NULL) {
		sys = lvm_lv_from_name(vg, volume);
		if (lvm_lv_activate(sys) < 0)
			write_logger(attention, 
					"Failed to activate volume %s!\n",
					volume);
		volume = va_arg(vols, const char *);
	}

	// Now that we activated lvs, free memory
	va_end(vols);
	lvm_vg_close(vg);
	lvm_quit(libh);

	// Let init handle remaining devices
	return 1;
}

/* Unlock main drive, 
 * activate logical volumes,
 * mount root 
 * switch root
 */

int main() {
	int fd = 0;
	if ((fd = set_console()) < 0)
		reboot(RB_HALT_SYSTEM);

	open_logger(debug);
	
	// Mount dev, proc, and sys
	if (set_mounts(MAKE_DEV |
			MAKE_PROC |
			MAKE_SYS) != 0) {
		// Exit immediately if inital mounts fails
		write_logger(critical, 
			"Unable to mount initial filesystems!\n");
		reboot(RB_HALT_SYSTEM);
	}

	// create /run/crypsetup with 755 permissions
	if (mkdir("/run/cryptsetup", 755) < 0)
		write_and_die("Failed to create mkdir /run/cryptsetup: %s\n",
							strerror(errno));

	if (mkdir("/mnt/crypt", 755) < 0) 
		write_and_die("Failed to create /mnt/crypt: %s\n",
						strerror(errno));

	if (mkdir("/mnt/key", 755) < 0)
		write_and_die("Failed to create /mnt/key: %s\n",
						strerror(errno));

	// Open the encrypted drive
	if (!open_drive("/dev/nvme0n1p3", "/dev/nvme0n1p2", "key.img", "key"))
		write_and_die("Failed to open encrypted drive!\n");	

	// activate vg SYSTEM, and lvs
	if (!set_lvm("SYS", "ROOT", "HOME", "SWAP", NULL))
		write_and_die("Failed to set up logical volumes!\n");

	if (mkdir("/mnt/root", 755) < 0)
		write_and_die("Failed to create /mnt/root: %s\n", 
						strerror(errno));

	// Mount root
	if (mount("/dev/mapper/SYS-ROOT", 
		"/mnt/root", 
		"ext4",
		0, 
		NULL) < 0) {
		write_and_die("Failed to mount root: %s\n", strerror(errno));
	}
	
	if (umount("/proc") < 0)
		write_and_die("Failed to umount /proc: %s\n", strerror(errno));
	
	if (umount("/sys") < 0)
		write_and_die("Failed to umount /sys: %s\n", strerror(errno));
	
	/* Since /dev/mapper/cryptroot and /dev/SYS/* are both active,
	 * I can't really umount /dev. However, I figure that maybe if I
	 * lazy umount, then maybe during the filesystem nuke, it might
	 * eventually umount.
	 */
	if (umount2("/dev", MNT_DETACH) < 0)
		write_and_die("Failed to umount /dev: %s\n", strerror(errno));

	close_console(fd);

	switch_root("/mnt/root", "/bin/sh");
}
