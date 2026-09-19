/* Native OS diagnostics. HSD callback handling lives in the original debug
 * module's native branch; no private FILE fields or PPC contexts are used. */
#include <sysdolphin/baselib/debug.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <execinfo.h>
#endif

void OSReport(char* format, ...)
{
    char local[2048];
    char* text = local;
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    int length = vsnprintf(local, sizeof(local), format, args);
    va_end(args);
    if (length < 0) {
        va_end(copy);
        return;
    }
    if ((size_t) length >= sizeof(local)) {
        text = malloc((size_t) length + 1);
        if (text != NULL) {
            vsnprintf(text, (size_t) length + 1, format, copy);
        } else {
            text = local;
            length = sizeof(local) - 1;
        }
    }
    va_end(copy);
    fwrite(text, 1, (size_t) length, stderr);
    HSD_NativeReport((const unsigned char*) text, (size_t) length);
    if (text != local) {
        free(text);
    }
}

void OSPanic(char* file, int line, char* format, ...)
{
    fprintf(stderr, "Melee panic at %s:%d: ", file, line);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
    fflush(stderr);
#ifdef __APPLE__
    void* frames[32];int count=backtrace(frames,32);
    backtrace_symbols_fd(frames,count,2);
#endif
    abort();
}
