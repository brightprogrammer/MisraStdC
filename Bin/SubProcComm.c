#include <Misra.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define READ_END  0
#define WRITE_END 1

int main(int argc, char** argv) {
    // create a pipe first
    i32 parent_read[2] = {0};
    i32 child_read[2]  = {0};
    if (pipe(parent_read) == -1 || pipe(child_read) == -1) {
        LOG_SYS_FATAL("Failed to create pipe");
    }

    const char* child_exec_path = argv[1];
    pid_t       child_pid       = fork();
    if (child_pid < 0) {
        LOG_SYS_FATAL("Fork failed");
    } else if (child_pid == 0) {
        LOG_INFO("Child process PID = {}", getpid());

        // redirect stdin and stdout to interact with parent process
        dup2(child_read[READ_END], STDIN_FILENO);
        dup2(parent_read[WRITE_END], STDOUT_FILENO);
        close(child_read[WRITE_END]);
        close(parent_read[READ_END]);

        execve(child_exec_path, argv + 1, NULL);
        LOG_SYS_FATAL("execve failed for child path {}", child_exec_path);
    } else {
        LOG_INFO("Parent process PID = {}, Child PID = {}", getpid(), child_pid);

        // redirect stdout and stdin to interact with child process
        dup2(child_read[WRITE_END], STDOUT_FILENO);
        dup2(parent_read[READ_END], STDIN_FILENO);
        close(child_read[READ_END]);
        close(parent_read[WRITE_END]);

        Str child_input = StrInitFromZstr("hello");
        FWriteFmtLn(stdout, "{}", child_input);

        // read complete child output
        Str child_output = StrInit();
        FReadFmt(stdin, "{}", child_output);

        // show that we actually read it
        LOG_INFO("[PARENT] Read child output ({} bytes) :\n{}", child_output.length, child_output);

        StrDeinit(&child_output);

        // wait for child process to exit
        i32 child_exit_status = 0;
        waitpid(child_pid, &child_exit_status, 0);
        LOG_INFO("[PARENT] Child process exited with status : {}", child_exit_status);
    }
    return EXIT_SUCCESS;
}
