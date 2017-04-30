#include <unistd.h>
#include <err.h>
#include <stdlib.h>

int main() {
	char *const eargs[] = { "./tiny2", "./tiny2", NULL };
	char *const eenv[] = { NULL };

	if (execve(eargs[0], eargs, eenv) != 0) {
		// The usage of the err() function here
		// does not affect the performance of the binaries
		// compiled by "gcc" or "musl-gcc".
		err(EXIT_FAILURE, "execve(%s)", eargs[0]);
	}

	return EXIT_FAILURE;
}
