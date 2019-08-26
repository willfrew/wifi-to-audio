/**
 * Credit: https://blog.boramalper.org/dropping-root-privileges-permanently-on-linux-in-c/
 */
#define _GNU_SOURCE  // for secure_getenv()
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>


int drop_root_privileges(void) {  // returns 0 on success and -1 on failure
	gid_t gid;
	uid_t uid;

	// no need to "drop" the privileges that you don't have in the first place!
	if (getuid() != 0) {
		return 0;
	}

	// when your program is invoked with sudo, getuid() will return 0 and you
	// won't be able to drop your privileges
	if ((uid = getuid()) == 0) {
		const char *sudo_uid = secure_getenv("SUDO_UID");
		if (sudo_uid == NULL) {
			printf("environment variable `SUDO_UID` not found\n");
			return -1;
		}
		errno = 0;
		uid = (uid_t) strtoll(sudo_uid, NULL, 10);
		if (errno != 0) {
			perror("under-/over-flow in converting `SUDO_UID` to integer");
			return -1;
		}
	}

	// again, in case your program is invoked using sudo
	if ((gid = getgid()) == 0) {
		const char *sudo_gid = secure_getenv("SUDO_GID");
		if (sudo_gid == NULL) {
			printf("environment variable `SUDO_GID` not found\n");
			return -1;
		}
		errno = 0;
		gid = (gid_t) strtoll(sudo_gid, NULL, 10);
		if (errno != 0) {
			perror("under-/over-flow in converting `SUDO_GID` to integer");
			return -1;
		}
	}

	if (setgid(gid) != 0) {
		perror("setgid");
		return -1;
	}
	if (setuid(uid) != 0) {
		perror("setgid");
		return -1;
	}

	// change your directory to somewhere else, just in case if you are in a
	// root-owned one (e.g. /root)
	if (chdir("/") != 0) {
		perror("chdir");
		return -1;
	}

	// check if we successfully dropped the root privileges
	if (setuid(0) == 0 || seteuid(0) == 0) {
		printf("could not drop root privileges!\n");
		return -1;
	}

	return 0;
}
