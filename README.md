Our tests show that fork() calls on 2.6.x Linux get slower as the parent process uses more memory. So do popen() and system().

This project is a much faster and more secure re-implementation of the Libc popen() and system() calls for Linux. The functions of this library can be used in currently existing source code which uses the standard popen() and system() functions. Small code modifications are required.

For more detailed information and benchmarks, please review the following blog pages:
  * http://blog.famzah.net/2009/11/20/a-much-faster-popen-and-system-implementation-for-linux/
  * http://blog.famzah.net/2009/11/20/fork-gets-slower-as-parent-process-use-more-memory/
  * http://blog.famzah.net/2013/05/28/nagios-improve-cpu-performance-with-popen_noshell/

Documentation, examples, unit tests and a performance benchmark tool are included in the source code.

Any comments, positive or negative, are welcome. Send them directly to my Gmail address, or use the "Issues" tracker here.
