#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void db_PrintThreadInfo(void);
static _Thread_local char output[1024];
static _Thread_local size_t length;

void OSReport(char* format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsnprintf(output + length, sizeof(output) - length, format, args);
    va_end(args);
    assert(n >= 0 && (size_t) n < sizeof(output) - length);
    length += (size_t) n;
}

static void* check(void* unused)
{
    (void) unused;
    length = 0;
    db_PrintThreadInfo();
    char* line = strstr(output, "base:");
    assert(line && strstr(line, "(peak unavailable)"));
    void *base, *end;
    size_t size, current;
    assert(sscanf(line, "base:%p, end:%p, size:%zu current:%zu",
                  &base, &end, &size, &current) == 4);
    assert(base == pthread_get_stackaddr_np(pthread_self()));
    assert(size == pthread_get_stacksize_np(pthread_self()));
    assert((uintptr_t) end == (uintptr_t) base - size);
    assert(current > 0 && current < size);
    return NULL;
}

int main(void)
{
    check(NULL);
    pthread_t worker;
    pthread_attr_t attr;
    assert(!pthread_attr_init(&attr));
    assert(!pthread_attr_setstacksize(&attr, 1024 * 1024));
    assert(!pthread_create(&worker, &attr, check, NULL));
    assert(!pthread_join(worker, NULL));
    assert(!pthread_attr_destroy(&attr));
    puts("Native thread diagnostics: main and custom worker stack bounds passed");
}
