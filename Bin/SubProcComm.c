#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>

// this program was verifed to work when executed with /bin/head
// the prgram writes something to child process and expect's the same thing echoed back
// so it can be verified that we got the same content
// executed like : Build/SubProcComm /bin/head -n 1
int main(int argc, char **argv, char **envp) {
    (void)argc;
    DefaultAllocator alloc = DefaultAllocatorInit();
    LogInit(false, &alloc.base);

    // create a new child process
    SysProc *proc = SysProcCreate(argv[1], argv + 1, envp, &alloc.base);

    // write something to it's stdout
    // (inlined SysProcWriteToStdinFmtLn because its header macro calls StrInit() without an allocator)
    {
        Str buf = StrInit(&alloc);
        StrWriteFmt(&buf, "value = {}", 42);
        StrPushBack(&buf, '\n');
        SysProcWriteToStdin(proc, &buf);
        StrDeinit(&buf);
    }

    // retrieve back the value
    i32 val = 0;
    // (inlined SysProcReadFromStdoutFmt for the same reason)
    {
        Str buf = StrInit(&alloc);
        SysProcReadFromStdout(proc, &buf);
        const char *in_ = buf.data;
        StrReadFmt(in_, "value = {}", val);
        StrDeinit(&buf);
    }

    // write the retrieved value to stdout (parent, not child)
    WriteFmtLn("got value = {}", val);

    // wait for program to exit for 1 second
    SysProcWaitFor(proc, 1000);

    // finally terminate
    SysProcDestroy(proc, &alloc.base);

    LogDeinit();
    DefaultAllocatorDeinit(&alloc);
    return 0;
}
