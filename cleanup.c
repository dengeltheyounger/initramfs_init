#include "cleanup.h"

// Create handlers for lvm and vg
static inline int set_handlers_lvm(lvm_t *vhandler, vg_t *ghandler) {
	*vhandler = lvm_init(NULL);

	// false
	if (!(*vhandler)) {
		return 0;
	}

	*ghandler = lvm_vg_open(*vhandler,
				"SYS",
				"r",
				0);

	if (!(*ghandler)) {
		lvm_quit(*vhandler);
		*vhandler = NULL;
		return 0;
	}

	return 1;
}

// Deactivate any logical volumes
// Return values can be used for error handling
void cleanup_lvm() {
	lvm_t vhandler = NULL;
	vg_t ghandler = NULL;
	lv_t lhandler = NULL;

	// The while loops allow us to break if set_handlers failed
	// Check for /dev/SYSTEM/ROOT
	while (access("/dev/mapper/SYS-ROOT", F_OK) == 0) {
		if (!set_handlers_lvm(&vhandler, &ghandler)) {
			break;
		}

		lhandler = lvm_lv_from_name(ghandler, "ROOT");
		lvm_lv_deactivate(lhandler);
		// Maybe something went wrong. Either way we want a quick break
		break;
	}

	// Check for /dev/SYSTEM/HOME
	while (access("/dev/mapper/SYS-HOME", F_OK) == 0) {
		// Create handlers if they have not already been created
		if (!vhandler) {
			if (!set_handlers_lvm(&vhandler, &ghandler)) {
				break;
			}
		}

		lhandler = lvm_lv_from_name(ghandler, "HOME");
		lvm_lv_deactivate(lhandler);
		break;
	}

	// Check for /dev/SYSTEM/SWAP
	while (access("/dev/mapper/SYS-SWAP", F_OK) == 0) {
		if (!vhandler) {
			if (!set_handlers_lvm(&vhandler, &ghandler)) {
				break;
			}
		}

		lhandler = lvm_lv_from_name(ghandler, "SWAP");
		lvm_lv_deactivate(lhandler);
		break;
	}

	
	// Free handlers if they were created
	if (vhandler) {
		lvm_vg_close(ghandler);
		lvm_quit(vhandler);
	}
}

// Deactivates drives, and umount if needed
void cleanup_cryptsetup() {
	struct crypt_device *handler = NULL;
	if (access("/dev/mapper/cryptkey", F_OK) == 0) {
		// Reinitilize cryptkey
		crypt_init_by_name(&handler, "cryptkey");
	
		// This may or may not fail.
		umount("/mnt/key");

		crypt_deactivate(handler, "cryptkey");

		// Delete handler for next mapping
		// Check in case init failed
		if (handler) {
			crypt_free(handler);
			handler = NULL;
		}
	}

	if (access("/dev/mapper/cryptboot", F_OK) == 0) {
		crypt_init_by_name(&handler, "cryptboot");

		umount("/mnt/crypt");
		
		crypt_deactivate(handler, "cryptboot");
		
		if (handler) {
			crypt_free(handler);
			handler = NULL;
		}
	}

	if (access("/dev/mapper/cryptroot", F_OK) == 0) {
		crypt_init_by_name(&handler, "cryptroot");

		crypt_deactivate(NULL, "cryptroot");

		// No more checks, so who cares if handler is NULL?
		if (handler)
			crypt_free(handler);
	}
}

/* I don't know of any easy way to check if a given directory is a mount point.
 * For that reason, I am going to call umount without checking. Worst case
 * scenario, it returns an error and wastes a few microseconds. Plus,
 * cleanup is only called after inital_mounts succeeds. For that reason,
 * we're probably okay
 */
void cleanup_mounts() {
	umount("/dev");
	umount("/proc");
	umount("/sys");
}

// Deactivate encrypted drives, volumes, and umount necessary filesystems
void cleanup() {
	// Umount root, deactivate volumes, close decrypted drives and exit
	umount("/mnt/root");
	cleanup_lvm();
	cleanup_cryptsetup();
	cleanup_mounts();
}

// Write error to logger, cleanup, and then turn off system
void write_and_die(char *msg, ...) {
	va_list args;
	va_start(args, msg);

	write_logger(0, msg, args);

	va_end(args);

	cleanup();

	// If we're in debug mode, then just halt system
	if (get_log_level() == debug) {
		reboot(RB_HALT_SYSTEM);
	} 
	
	// Otherwise power off system
	else {
		reboot(RB_POWER_OFF);
	}
}
