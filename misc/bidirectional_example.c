#include "popen_noshell.h"
#include <stdlib.h>
#include <err.h>

/*
 * XXX: NOTE: No error checking is performed, in order to simplify the example code.
 * Reference [issue #5]: https://github.com/famzah/popen-noshell/issues/5
 */

// use this struct to easily pass several parameters at once
struct child_arg_pass {
	int pipes[4];
	char **command;
};

int child_process(void *raw_arg) {
	struct child_arg_pass *arg;
	int fds[2];

	arg = (struct child_arg_pass *)raw_arg;

	// http://stackoverflow.com/a/3884402/198219
	fds[0] = (arg->pipes)[2];
	fds[1] = (arg->pipes)[1];

	close((arg->pipes)[0]);
	close((arg->pipes)[3]);

	// close STDIN and replace it with the pipe from the parent
	close(0);
	dup2(fds[0], 0);
	close(fds[0]);

	// close STDERR and replace it with the pipe from the parent
	close(1);
	dup2(fds[1], 1);
	close(fds[1]);

	// execute the command
	execvp((arg->command)[0], (char * const *)(arg->command));

	return 0;
}

int main() {
	pid_t pid;
	struct child_arg_pass arg;
	void *memory_to_free_on_child_exit;
	int fds[2];
	char c;

	// example Perl command: every character is repeated by surrounding it with single "x" chars
	char *cmd[] = { "perl", "-pi", "-e", "$| = 1; s/(.)/x$1x/g", (char *) NULL };
	arg.command = cmd;

	// http://stackoverflow.com/a/3884402/198219
	pipe(&(arg.pipes[0])); /* Parent read/child write pipe */
	pipe(&(arg.pipes[2])); /* Child read/parent write pipe */

	// fork() a child process using the popen_noshell() library
	pid = popen_noshell_vmfork(
		&child_process, &arg, &memory_to_free_on_child_exit
	);
	if (pid == -1) err(EXIT_FAILURE, "popen_noshell_vmfork()");

	/* Parent process */

	fds[0] = (arg.pipes)[0];
	fds[1] = (arg.pipes)[3];

	close((arg.pipes)[1]);
	close((arg.pipes)[2]);

	// write "123\n" to the child process
	write(fds[1], "123\n", 4);

	/*
	  Close the write pipe on the parent's side, so that the child process can exit.
	  Note that you are free to close the pipe at a later time too,
	  depending if you are going to write several times to the child process.
	*/
	close(fds[1]);

	// read the result from the child process and print it
	while (read(fds[0], &c, 1)) {
		printf("%c", c);
	}

	return 0;
}
