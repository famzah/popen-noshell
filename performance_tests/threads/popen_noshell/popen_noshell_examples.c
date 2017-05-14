#include "popen_noshell.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

int main() {
	FILE *fp;
	char buf[256];
	int status;
	struct popen_noshell_pass_to_pclose pclose_arg;
	int try;

	/* the command arguments used by popen_noshell() */
	char *exec_file = "./tiny2";
	char *arg1 = (char *) NULL; /* last element */
	char *argv[] = {exec_file, arg1}; /* NOTE! The first argv[] must be the executed *exec_file itself */
	
	for (try = 0; try < 100000; ++try) {
		fp = popen_noshell(exec_file, (const char * const *)argv, "r", &pclose_arg, 0);
		if (!fp) {
			err(EXIT_FAILURE, "popen_noshell()");
		}

		/*while (fgets(buf, sizeof(buf)-1, fp)) {
			printf("%s", buf);
		}*/

		status = pclose_noshell(&pclose_arg);
		if (status == -1) {
			err(EXIT_FAILURE, "pclose_noshell()");
		} else {
			if (status != 13) { // 13=SIGPIPE
				errx(EXIT_FAILURE, "./tiny2 failed");
			}
		}
	}

	return 0;
}
