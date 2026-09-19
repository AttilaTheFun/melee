#include "debug.h"

#include <stdio.h>

#include <dolphin/os.h>

#ifdef MELEE_NATIVE
#include <execinfo.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

static _Atomic(ReportCallback) reportCallback;
static _Atomic(PanicCallback) panicCallback;
static atomic_bool logEnabled;
static pthread_mutex_t reportMutex = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local int reporting;
static _Thread_local int panicking;

void HSD_LogInit(void)
{
    atomic_store(&logEnabled, 1);
}

/* OSReport supplies complete formatted bytes. Host stdio is not intercepted. */
void HSD_NativeReport(const unsigned char* bytes, size_t length)
{
    ReportCallback cb = atomic_load(&reportCallback);
    if (atomic_load(&logEnabled) && cb != NULL && !reporting) {
        reporting = 1;
        pthread_mutex_lock(&reportMutex);
        cb(bytes, length);
        pthread_mutex_unlock(&reportMutex);
        reporting = 0;
    }
}

void HSD_SetReportCallback(ReportCallback cb)
{
    atomic_store(&reportCallback, cb);
}

void HSD_SetPanicCallback(PanicCallback cb)
{
    atomic_store(&panicCallback, cb);
}

void __assert(char* file, u32 line, char* condition)
{
    OSReport("assertion \"%s\" failed\n", condition);
    HSD_Panic(file, line, condition);
}

void HSD_Panic(char* file, u32 line, char* message)
{
    if (panicking++) {
        abort();
    }
    HSD_NativePanicContext context = { .file = file, .line = line,
                                       .message = message };
    context.frame_count = backtrace(context.frames, 64);
    OSReport("%s in %s on line %u.\n", message, file, line);
    PanicCallback cb = atomic_load(&panicCallback);
    if (cb != NULL) {
        cb(&context);
    }
    /* Panic messages are data, even when they contain format specifiers. */
    OSPanic(file, line, "%s", message);
}
#else
struct DebugContext {
    OSContext context;
    u8 unk[0x10];
} HSD_Debug_804C2608;

static ReportCallback reportCallback;
static PanicCallback panicCallback;
static __io_proc logFunc;

#ifdef MUST_MATCH
#pragma peephole off
#endif

static int report_func(__file_handle arg0, unsigned char* arg1, size_t* arg2,
                       __idle_proc arg3)
{
    if (reportCallback != NULL) {
        reportCallback(arg1, *arg2);
    }
    logFunc(arg0, arg1, arg2, arg3);
    return 0;
}

void HSD_LogInit(void)
{
    if (logFunc == NULL) {
        logFunc = stdout->write_proc;
    }
    stdout->write_proc = report_func;
    stdout->state.error = 0;
}

void __assert(char* str, u32 arg1, char* arg2)
{
    OSReport("assertion \"%s\" failed", arg2);
    HSD_Panic(str, arg1, "");
}

void HSD_Panic(char* arg0, u32 line, char* arg2)
{
    if (panicCallback != NULL) {
        OSSaveContext(&HSD_Debug_804C2608.context);
        OSReport("%s in %s on line %d.\n", arg2, arg0, line);
        panicCallback(&HSD_Debug_804C2608.context);
    }
    OSPanic(arg0, line, arg2);
}

void HSD_SetReportCallback(ReportCallback cb)
{
    reportCallback = cb;
}

void HSD_SetPanicCallback(PanicCallback cb)
{
    panicCallback = cb;
}

#endif /* MELEE_NATIVE */
