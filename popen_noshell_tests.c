/*
 * popen_noshell: A faster implementation of popen() and system() for Linux.
 * Copyright (c) 2009 Ivan Zahariev (famzah)
 * Version: 1.0
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include "popen_noshell.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

/***************************************************
 * popen_noshell C unit test and use-case examples *
 ***************************************************
 *
 * Compile and test via:
 * 	gcc -Wall popen_noshell.c popen_noshell_tests.c -o popen_noshell_tests && ./popen_noshell_tests
 *	# XXX: also run the examples in "popen_noshell_examples.c"
 *
 * Compile for debugging by Valgrind via:
 * 	gcc -Wall -g -DPOPEN_NOSHELL_VALGRIND_DEBUG popen_noshell.c popen_noshell_tests.c -o popen_noshell_tests
 * Then start under Valgrind via:
 * 	valgrind -q --tool=memcheck --leak-check=yes --show-reachable=yes --track-fds=yes ./popen_noshell_tests
 * If you want to filter Valgrind false reports about 0 opened file descriptors, add the following at the end:
 *	2>&1|egrep -v '^==[[:digit:]]{1,5}==( | FILE DESCRIPTORS: 0 open at exit.)$'
 */

int do_unit_tests_ignore_stderr;

char *bin_bash = "/bin/bash";
char *bin_true = "/bin/true";
char *bin_cat  = "/bin/cat";
char *bin_echo = "/bin/echo";

void satisfy_open_FDs_leak_detection_and_exit() {
	/* satisfy Valgrind FDs leak detection for the parent process */
	if (fflush(stdout) != 0) err(EXIT_FAILURE, "fflush(stdout)");
	if (fflush(stderr) != 0) err(EXIT_FAILURE, "fflush(stderr)");
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	exit(0);
}

void assert_string(char *expected, char *got, const char *assert_desc) {
	if (strcmp(expected, got) != 0) errx(EXIT_FAILURE, "%s: Expected '%s', got '%s'", assert_desc, expected, got);
}

void assert_int(int expected, int got, const char *assert_desc) {
	if (expected != got) errx(EXIT_FAILURE, "%s: Expected %d, got %d", assert_desc, expected, got);
}

void assert_status_not_internal_error(int status) {
	assert_int(1, status >= 0, "assert_status_not_internal_error");
}

void assert_status_signal(int signal, int status) {
	assert_status_not_internal_error(status);
	assert_int(signal, status & 127, "assert_status_signal");
}

void assert_status_exit_code(int code, int status) {
	assert_status_not_internal_error(status);
	assert_status_signal(0, status);
	assert_int(code, status >> 8, "assert_status_exit_code");
}

FILE *safe_popen_noshell(const char *file, const char * const *argv, const char *type, struct popen_noshell_pass_to_pclose *pclose_arg, int stderr_mode) {
	FILE *fp = popen_noshell(file, argv, type, pclose_arg, stderr_mode);
	if (!fp) err(EXIT_FAILURE, "popen_noshell");
	return fp;
}

void safe_pclose_noshell(struct popen_noshell_pass_to_pclose *arg) {
	int status;

	status = pclose_noshell(arg);
	assert_status_exit_code(0, status);
}

void unit_test(int reading, char *argv[], char *expected_string, int expected_signal, int expected_exit_code) {
	FILE *fp;
	char buf[256];
	int status;
	struct popen_noshell_pass_to_pclose pclose_arg;
	char *received;
	size_t received_size;
	int stderr_mode = (do_unit_tests_ignore_stderr ? 1 : 0);

	fp = safe_popen_noshell(argv[0], (const char * const *)argv, reading ? "r" : "w", &pclose_arg, stderr_mode);

	if (reading) {
		received_size = strlen(expected_string) + 256; // so that we can store a bit longer strings that we expected and discover the mismatch
		received = alloca(received_size); // use alloca() or else the fork()'ed child will generate a Valgrind memory leak warning if exec() fails
		if (!received) err(EXIT_FAILURE, "alloca");
		memset(received, 0, received_size); // ensure a terminating null

		while (fgets(buf, sizeof(buf) - 1, fp)) {
			strncat(received, buf, received_size - strlen(received) - 2);
		}

		assert_string(expected_string, received, "Received string");
	}

	status = pclose_noshell(&pclose_arg);
	if (status == -1) {
		err(EXIT_FAILURE, "pclose_noshell()");
	} else {
		if (expected_signal != 0) {
			assert_status_signal(expected_signal, status);
		} else {
			assert_status_exit_code(expected_exit_code, status);
		}
	}

	//free(received); // memory allocated by alloca() cannot be free()'d
}

void do_unit_tests() {
	int test_num = 0;
	int more_to_test = 1;

	do {
		++test_num;
		switch (test_num) {
			case 1: {
				char *argv[] = {"/", NULL};
				unit_test(1, argv, "", 0, 255); // failed to execute binary (status code is -1, STDOUT is empty, STDERR text)
				break;
			}
			case 2: {
				char *argv[] = {bin_bash, "-c", "ulimit -t 1 && while [ 1 ]; do let COUNTER=COUNTER+1; done;", NULL};
				unit_test(1, argv, "", 9, 5); // process signalled with 9 due to CPU limit (STDOUT/ERR are empty)
				break;
			}
			case 3: {
				char *argv[] = {bin_bash, "-c", "sleep 1; exit 1", NULL};
				unit_test(1, argv, "", 0, 1); // process exited with value 1   (STDOUT/ERR are empty)
				break;
			}
			case 4: {
				char *argv[] = {bin_bash, "-c", "exit 255", NULL};
				unit_test(1, argv, "", 0, 255); // process exited with value 255 (STDOUT/ERR are empty)
				break;
			}
			case 5: {
				char *argv[] = {bin_bash, "-c", "echo \"some err string\" 1>&2; exit 111", NULL};
				unit_test(1, argv, "", 0, 111); // process exited with value 111 (STDERR text, STDOUT is empty)
				break;
			}
			case 6: {
				char *argv[] = {bin_bash, "-c", "echo -en \"some err\\nstring v2\" 1>&2; echo -en \"some\\ngood text\"; exit 0", NULL};
				unit_test(1, argv, "some\ngood text", 0, 0); // process exited with value 0 (STDERR text, STDOUT text)
				break;
			}
			case 7: {
				char *argv[] = {bin_bash, "-c", "echo -e \"\" 1>&2; echo -e \"\"; exit 3", NULL};
				unit_test(1, argv, "\n", 0, 3); // process exited with value 3 (STDERR text, STDOUT text)
				break;
			}
			case 8: {
				char *argv[] = {bin_bash, NULL};
				unit_test(1, argv, "", 0, 0); // process exited with value 0 (single argument, STDOUT/ERR are empty)
				break;
			}
			case 9: {
				char *argv[] = {bin_true, NULL};
				unit_test(1, argv, "", 0, 0); // process exited with value 0 (single argument, STDOUT/ERR are empty)
				break;
			}
			case 10: {
				char *argv[] = {bin_cat, NULL}; // cat expects an input from STDIN
				unit_test(1, argv, "", 0, 0); // process exited with value 0 (single argument, STDOUT/ERR are empty)
				break;
			}
			case 11: {
				char *argv[] = {bin_echo, NULL};
				unit_test(1, argv, "\n", 0, 0); // process exited with value 0 (single argument, STDERR is empty, STDOUT text)
				break;
			}
			default:
				more_to_test = 0;
				break;
		}
	} while (more_to_test);

	assert_int(11, test_num - 1, "Test count");
}

void issue_4_double_free() { // make sure we don't re-introduce this bug
	FILE *fp;
	struct popen_noshell_pass_to_pclose pclose_arg;

	fp = popen_noshell_compat("Some b&d command", "r", &pclose_arg);
	if (fp) {
		errx(EXIT_FAILURE,
			"issue_4_double_free(): popen_noshell_compat() must have returned NULL."
		);
	}
}

void issue_7_missing_cloexec() {
	struct popen_noshell_pass_to_pclose pc1, pc2;
	const char *cmd[] = {bin_cat, NULL};
	
	// we don't even need the returned FILE *
	safe_popen_noshell(cmd[0], cmd, "w", &pc1, 0);
	safe_popen_noshell(cmd[0], cmd, "w", &pc2, 0);
	safe_pclose_noshell(&pc1);
	safe_pclose_noshell(&pc2);
}

int _issue_8_mute_stderr() {
	int fd2;
	int saved_stderr_fd;

	saved_stderr_fd = open("/dev/null", O_WRONLY); // get a valid, free "fd" number
	if (saved_stderr_fd < 0) {
		err(EXIT_FAILURE, "open(\"/dev/null\")");
	}
	if (dup2(STDERR_FILENO, saved_stderr_fd) < 0) { // store the current STDERR "fd" info
		err(EXIT_FAILURE, "dup2(stderr to saved_stderr_fd)");
	}
	/*if (close(STDERR_FILENO) < 0) { // mute, or else we display the STDERR from the child
		err(EXIT_FAILURE, "close(stderr)");
	}*/

	fd2 = open("/dev/null", O_WRONLY);
	if (fd2 < 0) {
		err(EXIT_FAILURE, "open(\"/dev/null\")");
	}
	if (dup2(fd2, STDERR_FILENO) < 0) { // mute, or else we display the STDERR from the child
		#define _temp_err_msg "dup2(/dev/null to stderr) failed\n"

		/* we may not have STDERR, so write() to the saved "fd" first */
		write(saved_stderr_fd, _temp_err_msg, strlen(_temp_err_msg));

		err(EXIT_FAILURE, _temp_err_msg); 

		#undef _temp_err_msg
	}
	close(fd2); // satisfy Valgrind

	return saved_stderr_fd;
}

void _issue_8_restore_stderr(int saved_stderr_fd) {
	if (dup2(saved_stderr_fd, STDERR_FILENO) < 0) {
		#define _temp_err_msg "dup2(restore stderr) failed\n"

		/* we don't have STDERR, so write() to the saved "fd" first */
		write(saved_stderr_fd, _temp_err_msg, strlen(_temp_err_msg));

		err(EXIT_FAILURE, _temp_err_msg); 

		#undef _temp_err_msg
	}
	close(saved_stderr_fd); // satisfy Valgrind
}

int _issue_8_call_popen(int stderr_mode) {
	struct popen_noshell_pass_to_pclose pc;
	const char *cmd[] = {bin_cat, NULL};

	safe_popen_noshell(cmd[0], cmd, "w", &pc, stderr_mode);
	return pclose_noshell(&pc);
}

void issue_8_stderr_mode_test_invalid_mode() {
	int last_valid_mode = 2;
	int stderr_mode;
	int status;
	pid_t pid, ret;
	int saved_stderr_fd;

	for (stderr_mode = 0; stderr_mode <= last_valid_mode; ++stderr_mode) { /* no errors expected */
		status = _issue_8_call_popen(stderr_mode);
		assert_status_exit_code(0, status);
	}
	/* XXX: stderr_mode is now invalid and equals "last_valid_mode + 1" */

	pid = fork(); // fork, because we're doing some funny stuff with STDERR
	if (pid == -1) {
		err(EXIT_FAILURE, "fork()");
	}

	if (pid == 0) { // forked process
		saved_stderr_fd = _issue_8_mute_stderr(); // temporarily
		// XXX: When the popen_noshell() call fails before its exec() phase,
		// XXX: the opened "saved_stderr_fd" is detected as leaked by Valgrind,
		// XXX: but this is unavoidable.
		status = _issue_8_call_popen(stderr_mode /* invalid */);
		_issue_8_restore_stderr(saved_stderr_fd);
		assert_status_exit_code(254, status);
		satisfy_open_FDs_leak_detection_and_exit();
	}
	
	/* parent continues here */

	ret = waitpid(pid, &status, 0);
	if (ret != pid) {
		errx(EXIT_FAILURE, "waitpid() failed");
	}
	if (status != 0) {
		errx(EXIT_FAILURE,
			"issue_8_stderr_mode_test_invalid_mode(): failed for %d", stderr_mode
		);
	}
}

void _issue_8_assert_no_more_output(char *buf, size_t buf_size, FILE *fp, int stderr_mode) {
	if (fgets(buf, buf_size - 1, fp) == NULL && feof(fp)) {
		// all good: nothing read, and we got EOF immediately
	} else {
		errx(EXIT_FAILURE,
			"issue_8_stderr_mode_test_option_2():"\
			" mode=%d, and we have output or error?",
			stderr_mode
		);
	}
}

void issue_8_stderr_mode_test_option_2() {
	struct popen_noshell_pass_to_pclose pc;
	const char *cmd[] = {bin_bash, "-c", "echo STDERR message >&2", NULL};
	int stderr_mode;
	FILE *fp;
	char buf[256];

	for (stderr_mode = 1; stderr_mode <= 2; ++stderr_mode) {
		fp = safe_popen_noshell(cmd[0], cmd, "r", &pc, stderr_mode);

		if (stderr_mode == 1) {
			// we ignore STDERR, so we don't expect any output
			_issue_8_assert_no_more_output(buf, sizeof(buf), fp, stderr_mode);
		} else if (stderr_mode == 2) {
			if (fgets(buf, sizeof(buf) - 1, fp) == NULL) {
				errx(EXIT_FAILURE,
					"issue_8_stderr_mode_test_option_2():"\
					" mode=%d and we have NO output, or got an error?",
					stderr_mode
				);
			}
			assert_string("STDERR message\n", buf,
				"issue_8_stderr_mode_test_option_2()"
			);
			_issue_8_assert_no_more_output(buf, sizeof(buf), fp, stderr_mode);
		} else {
			errx(EXIT_FAILURE, "never reached");
		}

		safe_pclose_noshell(&pc);
	}
}

void proceed_to_standard_unit_tests() {
	do_unit_tests_ignore_stderr = 1; /* do we ignore STDERR from the executed commands? */

	popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_CLONE); /* the default one */
	do_unit_tests();
	popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_FORK);
	do_unit_tests();
}

void proceed_to_issues_tests() {
	issue_4_double_free();
	issue_7_missing_cloexec();
	issue_8_stderr_mode_test_invalid_mode();
	issue_8_stderr_mode_test_option_2();
}

int main() {
	proceed_to_standard_unit_tests();
	proceed_to_issues_tests();

	printf("Tests passed OK.\n");

	satisfy_open_FDs_leak_detection_and_exit();
	return 0; /* never reached, because we did exit(0) already */
}
