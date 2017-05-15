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
	int done; // XXX: implement this using pipe(), in order to be able to use epoll()
};

pthread_attr_t p_attr; // used for all threads
int finished_threads = 0; // quick hack to wait for all threads to finish

#define use_pthread_attr 1 /* try two different methods to configure a thread as "detached" */

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

	pns_call->done = 1;

	++finished_threads;

	return NULL;
}

void do_vfork_in_thread() {
	int thr_errno;
	struct pns_call_meta pns_call;

	pns_call.done = 0;
	
	if (use_pthread_attr) {
		thr_errno = pthread_create(&(pns_call.thread), &p_attr, vfork_thread, &pns_call);
	} else {
		thr_errno = pthread_create(&(pns_call.thread), NULL, vfork_thread, &pns_call);
	}
	if (thr_errno != 0) {
		errx(EXIT_FAILURE, "pthread_create(): %s", strerror(thr_errno));
	}

	if (!use_pthread_attr) {
		if (pthread_detach(pns_call.thread) != 0) {
			errx(EXIT_FAILURE, "pthread_detach()");
		}
	}

	/*while (!pns_call.done) {
		printf("Parent: thread is not done, yet\n");
		sleep(1);
	}*/
}

int main() {
	int i;

	// auto-reap exited child processes (only for this simple benchmark test)
	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
		err(EXIT_FAILURE, "signal()");
	}

	// we don't need to wait for the thread to exit by calling pthread_join()
	if (pthread_attr_init(&p_attr) != 0) {
		err(EXIT_FAILURE, "pthread_attr_init()");
	}
	if (pthread_attr_setdetachstate(&p_attr, PTHREAD_CREATE_DETACHED) != 0) {
		err(EXIT_FAILURE, "pthread_attr_setdetachstate()");
	}

	for (i = 0; i < 100000; ++i) {
		do_vfork_in_thread();
	}

	while (finished_threads != 100000) {
		sleep(0.05);
	}

	return 0;
}
