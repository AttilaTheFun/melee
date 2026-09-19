#include <sysdolphin/baselib/debug.h>
#include <assert.h>
#include <fenv.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/hsd_393C.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char db_build_timestamp[] = "native diagnostics regression";

static unsigned char captured[8192];
static size_t captured_size;
static int calls;
static int panic_pipe;

static void capture(const unsigned char* bytes, size_t length)
{
    assert(length <= sizeof(captured));
    memcpy(captured, bytes, length);
    captured_size = length;
    ++calls;
    /* A callback can log without recursively invoking itself. */
    OSReport("nested report\n");
}

static void panic_callback(const HSD_NativePanicContext* context)
{
    unsigned char valid = strcmp(context->file, "panic-test") == 0 &&
        context->line == 123 && strcmp(context->message, "literal %s %n") == 0 &&
        context->frame_count > 0 && context->frame_count <= 64 &&
        (uintptr_t) context->frames[0] > UINT32_MAX;
    if (write(panic_pipe, &valid, 1) != 1) {
        _exit(90);
    }
}

static void aborted(int signal_number)
{
    _exit(signal_number == SIGABRT ? 86 : 91);
}

int main(void)
{
    assert(freopen("/dev/null", "w", stderr) != NULL);
    HSD_SetReportCallback(capture);
    OSReport("before init");
    assert(calls == 0);
    HSD_LogInit();
    HSD_LogInit();
    OSReport("value %d %s", 42, "ok");
    assert(calls == 1 && captured_size == 11);
    assert(memcmp(captured, "value 42 ok", 11) == 0);
    OSReport("a%cb", 0);
    assert(calls == 2 && captured_size == 3);
    assert(memcmp(captured, "a\0b", 3) == 0);
    char large[6001];
    memset(large, 'x', sizeof(large) - 1);
    large[6000] = 0;
    OSReport("%s", large);
    assert(calls == 3 && captured_size == 6000);
    assert(memcmp(captured, large, 6000) == 0);
    HSD_SetReportCallback(NULL);
    OSReport("disabled");
    assert(calls == 3);

    feraiseexcept(FE_INVALID | FE_DIVBYZERO);
    assert(fetestexcept(FE_ALL_EXCEPT) != 0);
    db_ClearFPUExceptions();
    assert(fetestexcept(FE_ALL_EXCEPT) == 0);
    db_SetupCrashHandler();
    OSReport("first\nsecond\n");
    s32 rows = 0;
    hsd_80393E34(NULL, &rows);
    assert(rows == 2);
    unsigned char ring[128];
    hsd_80393DA0(ring, sizeof(ring));
    OSReport("abc\r\n");
    assert(memcmp(ring, "abc\3", 4) == 0);
    assert(hsd_80393D2C(0) == 1);
    OSReport("disabled ring");
    assert(ring[4] == 0);

    int pipe_fds[2];
    assert(pipe(pipe_fds) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(pipe_fds[0]);
        panic_pipe = pipe_fds[1];
        signal(SIGABRT, aborted);
        HSD_SetPanicCallback(panic_callback);
        HSD_Panic("panic-test", 123, "literal %s %n");
    }
    close(pipe_fds[1]);
    unsigned char valid = 0;
    assert(read(pipe_fds[0], &valid, 1) == 1 && valid == 1);
    close(pipe_fds[0]);
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 86);
    puts("Native diagnostics: formatted reports, recursion guard, host panic backtrace and fatal termination passed");
    return 0;
}
