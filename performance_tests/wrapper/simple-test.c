#include <unistd.h>
#include <signal.h>
#include <err.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>

int main(int argc, char *const argv[]) {
	int i;
	pid_t pid;

	if (argc != 2) {
		errx(EXIT_FAILURE, "Usage: %s FILE_TO_EXEC", argv[0]);
	}

	char *const eargs[] = { argv[1], argv[1], NULL };
	char *const eenv[] = { NULL };

	// auto-reap exited child processes
	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
		err(EXIT_FAILURE, "signal()");
	}

	// the time-critical code follows
	for (i = 0; i < 100000; ++i) {
		pid = vfork();
		if (pid < 0) {
			err(EXIT_FAILURE, "vfork()");
		}
		if (pid == 0) { // child
			if (execve(eargs[0], eargs, eenv) != 0) {
				err(EXIT_FAILURE, "execl()"); // you must NOT do this in vfork()!
				// the err() is here only for debug while in development
			}
			_exit(1);
		}
		// parent process:
		// - the parent is suspended until the vfork()'ed child calls execve() or _exit()
		// - the exited child process is auto-reaped because we ignore SIGCHLD
	}

	return 0;
}
