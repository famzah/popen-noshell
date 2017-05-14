This is a research try and is work in progress. In regards to [issue #11](https://github.com/famzah/popen-noshell/issues/11).

The idea is to remove the flag CLONE_VFORK, because it stops the parent process until the
vfork()'ed child calls execve().

Simply removing the CLONE_VFORK flag is not sufficient, because the vfork()'ed child uses the
same TLS (Thread-Local Storage) where "errno", for example, resides. The child process can,
therefore, modify the "errno" variable in the parent asynchronously, which is not desirable.

In order to have our own TLS and not stop the parent process, we start a helper thread. Its
only task is to call vfork(). This way the vfork()'ed child process stops the helper thread,
and the real parent process continues to run. At the same time, each thread has its own TLS.

See [issue #11](https://github.com/famzah/popen-noshell/issues/11) for more details.
