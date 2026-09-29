#ifdef MELEE_NATIVE
#include "db.h"
#include <fenv.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#include <execinfo.h>
#endif
#include <unistd.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/hsd_393C.h>

void db_ClearFPUExceptions(void)
{
    feclearexcept(FE_ALL_EXCEPT);
}

static void fn_HSDPanicHandler(const HSD_NativePanicContext* ctx)
{
    OSReport("%s\n", db_build_timestamp);
#ifdef __EMSCRIPTEN__
    (void)ctx;
    emscripten_log(EM_LOG_ERROR | EM_LOG_C_STACK, "Melee panic");
#else
    backtrace_symbols_fd(ctx->frames, ctx->frame_count, STDERR_FILENO);
#endif
}

void db_SetupCrashHandler(void)
{
    static u8 log_buffer[0x2000];
    HSD_LogInit();
    hsd_80393DA0(log_buffer, sizeof(log_buffer));
    HSD_SetPanicCallback(fn_HSDPanicHandler);
}
#else
#include <stdarg.h>

#include "db.h"
#include <dolphin/base/PPCArch.h>
#include <dolphin/db.h>
#include <dolphin/os.h>
#include <melee/lb/lb_0195.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/debugconsole_main.h>
#include <sysdolphin/baselib/hsd_393C.h>
#include <sysdolphin/baselib/video.h>

/* 228AB4 */ static void fn_HSDPanicHandler(OSContext* ctx);
/* 228B28 */ static void fn_OSErrorHandler(u16 error, OSContext* ctx, ...);

void db_ClearFPUExceptions(void)
{
    OSContext* ctx;

    PPCMtmsr(PPCMfmsr() | 0x900);
    ctx = OSGetCurrentContext();
    OSSaveFPUContext(ctx);
    ctx->fpscr &= 0xFFFFF;
    OSLoadFPUContext(ctx);
}

static void fn_HSDPanicHandler(OSContext* ctx)
{
    HSD_VISetUserPreRetraceCallback(NULL);
    HSD_VISetUserPostRetraceCallback(NULL);
    lb_80019A48();
    OSReport("%s\n", db_build_timestamp);
    Exception_ReportStackTrace(ctx, 0x10);
    hsd_80397DFC(0x1388);
    Exception_StoreDebugLevel(DbLevel);
    hsd_80397DA4(ctx);
}

static void fn_OSErrorHandler(u16 error, OSContext* ctx, ...)
{
    int dsisr, dar;

    va_list va;
    va_start(va, ctx);

    dsisr = va_arg(va, int);
    dar = va_arg(va, int);

    HSD_VISetUserPreRetraceCallback(NULL);
    HSD_VISetUserPostRetraceCallback(NULL);
    lb_80019A48();
    OSReport("%s\n", db_build_timestamp);
    Exception_ReportStackTrace(ctx, 0x10);
    Exception_ReportCodeline(error, dsisr, dar, ctx);
    hsd_80397DFC(0x1388);
    Exception_StoreDebugLevel(DbLevel);
    hsd_80397DA4(ctx);

    va_end(va);
}

void db_SetupCrashHandler(void)
{
    u16 x;
    if (DBIsDebuggerPresent() == 0) {
        void* mem = OSAllocFromArenaLo(0x2000, 4);
        hsd_80393DA0(mem, 0x2000);
        HSD_SetPanicCallback((PanicCallback) fn_HSDPanicHandler);
        for (x = 0; x < 16; x++) {
            switch (x) {
            case 4:
            case 7:
            case 8:
            case 9:
                break;
            default:
                OSSetErrorHandler(x, fn_OSErrorHandler);
            }
        }
    }
}

#endif /* MELEE_NATIVE */
