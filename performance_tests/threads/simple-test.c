#include <pthread.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

struct pns_call_meta {
	pthread_t thread;
	int joinable; // XXX: implement this using pipe(), in order to be able to use epoll()
};

void *vfork_thread(void *arg) {
	struct pns_call_meta *pns_call = (struct pns_call_meta *) arg;
	pid_t pid;
	char *const eargs[] = { "./tiny2", "./tiny2", NULL };
	char *const eenv[] = { NULL };

	pid = vfork();
	if (pid < 0) {
		warn("vfork()");
		return NULL;
	}
	if (pid == 0) { // child process; the parent thread is suspended now
		if (execve(eargs[0], eargs, eenv) != 0) {
			err(EXIT_FAILURE, "execl()"); // you must NOT do this in vfork()!
			// the err() is here only for debug while in development
		}
		_exit(1);
	}

	// parent process:
	// - the parent is suspended until the vfork()'ed child calls execve() or _exit()
	// - the exited child process is auto-reaped because we ignore SIGCHLD

	pns_call->joinable = 1;

	return NULL;
}

void do_vfork_in_thread() {
	int thr_errno;
	struct pns_call_meta pns_call;

	pns_call.joinable = 0;

	thr_errno = pthread_create(&(pns_call.thread), NULL, vfork_thread, &pns_call);
	if (thr_errno != 0) {
		errx(EXIT_FAILURE, "pthread_create(): %s", strerror(thr_errno));
	}

	/*while (!pns_call.joinable) {
		printf("Parent: thread is not joinable, yet\n");
		sleep(1);
	}*/

	thr_errno = pthread_join(pns_call.thread, NULL);
	if (thr_errno != 0) {
		errx(EXIT_FAILURE, "pthread_join(): %s", strerror(thr_errno));
	}
}

int main() {
	int i;

	// auto-reap exited child processes (only for this simple benchmark test)
	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
		err(EXIT_FAILURE, "signal()");
	}

	//for (i = 0; i < 100000; ++i) {
	for (i = 0; i < 1; ++i) {
		do_vfork_in_thread();
	}

	return 0;
}
