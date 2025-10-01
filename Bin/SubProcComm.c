#include <Misra.h>

// this program was verifed to work when executed with /bin/head
// the prgram writes something to child process and expect's the same thing echoed back
// so it can be verified that we got the same content
// executed like : Build/SubProcComm /bin/head -n 1
int main(int argc, char **argv, char **envp) {
    // create a new child process
    SysProc *proc = SysProcCreate(argv[1], argv + 1, envp);

    // write something to it's stdout
    SysProcWriteToStdinFmtLn(proc, "value = {}", 42);

    // retrieve back the value
    i32 val = 0;
    SysProcReadFromStdoutFmt(proc, "value = {}", val);

    // write the retrieved value to stdout (parent, not child)
    WriteFmtLn("got value = {}", val);

    // wait for program to exit for 1 second
    SysProcWaitFor(proc, 1000);

    // finally terminate
    SysProcDestroy(proc);
}
