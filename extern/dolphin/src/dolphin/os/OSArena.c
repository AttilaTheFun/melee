#ifdef MELEE_NATIVE
#include <dolphin/os.h>
#include <pthread.h>
#include <stdint.h>

/* Bounds are supplied by the host before game initialization. This allocator
 * reserves ranges; it never owns or frees the underlying host allocation. */
static uintptr_t arenaLo = UINTPTR_MAX;
static uintptr_t arenaHi;
static pthread_mutex_t arenaMutex = PTHREAD_MUTEX_INITIALIZER;

static void checkArena(void)
{
    if (arenaLo == UINTPTR_MAX || arenaHi < arenaLo) {
        OSPanic(__FILE__, __LINE__, "Native OS arena is not initialized or has invalid bounds");
    }
}

void* OSGetArenaLo(void)
{
    pthread_mutex_lock(&arenaMutex);
    checkArena();
    void* result = (void*) arenaLo;
    pthread_mutex_unlock(&arenaMutex);
    return result;
}

void* OSGetArenaHi(void)
{
    pthread_mutex_lock(&arenaMutex);
    checkArena();
    void* result = (void*) arenaHi;
    pthread_mutex_unlock(&arenaMutex);
    return result;
}

void OSSetArenaLo(void* value)
{
    pthread_mutex_lock(&arenaMutex);
    arenaLo = (uintptr_t) value;
    pthread_mutex_unlock(&arenaMutex);
}

void OSSetArenaHi(void* value)
{
    pthread_mutex_lock(&arenaMutex);
    arenaHi = (uintptr_t) value;
    pthread_mutex_unlock(&arenaMutex);
}

void* OSAllocFromArenaLo(u32 size, u32 align)
{
    if (!align || (align & (align - 1))) {
        return NULL;
    }
    uintptr_t mask = (uintptr_t) align - 1;
    void* result = NULL;
    pthread_mutex_lock(&arenaMutex);
    checkArena();
    if (arenaLo <= UINTPTR_MAX - mask) {
        uintptr_t start = (arenaLo + mask) & ~mask;
        if (start <= arenaHi && size <= arenaHi - start) {
            uintptr_t end = start + size;
            if (end <= UINTPTR_MAX - mask) {
                end = (end + mask) & ~mask;
                if (end <= arenaHi) {
                    arenaLo = end;
                    result = (void*) start;
                }
            }
        }
    }
    pthread_mutex_unlock(&arenaMutex);
    return result;
}

void* OSAllocFromArenaHi(u32 size, u32 align)
{
    if (!align || (align & (align - 1))) {
        return NULL;
    }
    uintptr_t mask = (uintptr_t) align - 1;
    void* result = NULL;
    pthread_mutex_lock(&arenaMutex);
    checkArena();
    uintptr_t end = arenaHi & ~mask;
    if (end >= arenaLo && size <= end - arenaLo) {
        uintptr_t start = (end - size) & ~mask;
        if (start >= arenaLo) {
            arenaHi = start;
            result = (void*) start;
        }
    }
    pthread_mutex_unlock(&arenaMutex);
    return result;
}
#else
#include <dolphin.h>
#include <dolphin/os.h>

static void* __OSArenaHi;
static void* __OSArenaLo = (void*) -1;

void* OSGetArenaHi()
{
    ASSERTMSGLINE(0x37, (u32) __OSArenaLo != -1,
                  "OSGetArenaHi(): OSInit() must be called in advance.");
    ASSERTMSGLINE(0x39, (u32) __OSArenaLo <= (u32) __OSArenaHi,
                  "OSGetArenaHi(): invalid arena (hi < lo).");
    return __OSArenaHi;
}

void* OSGetArenaLo()
{
    ASSERTMSGLINE(0x49, (u32) __OSArenaLo != -1,
                  "OSGetArenaLo(): OSInit() must be called in advance.");
    ASSERTMSGLINE(0x4B, (u32) __OSArenaLo <= (u32) __OSArenaHi,
                  "OSGetArenaLo(): invalid arena (hi < lo).");
    return __OSArenaLo;
}

void OSSetArenaHi(void* newHi)
{
    __OSArenaHi = newHi;
}

void OSSetArenaLo(void* newLo)
{
    __OSArenaLo = newLo;
}

void* OSAllocFromArenaLo(u32 size, u32 align)
{
    void* ptr;
    u8* arenaLo;

    ptr = OSGetArenaLo();
    arenaLo = ptr = (void*) ROUND(ptr, align);
    arenaLo += size;
    arenaLo = (u8*) ROUND(arenaLo, align);
    OSSetArenaLo(arenaLo);
    return ptr;
}

void* OSAllocFromArenaHi(u32 size, u32 align)
{
    void* ptr;
    u8* arenaHi;

    arenaHi = OSGetArenaHi();
    arenaHi = (u8*) TRUNC(arenaHi, align);
    arenaHi -= size;
    arenaHi = ptr = (void*) TRUNC(arenaHi, align);
    OSSetArenaHi(arenaHi);
    return ptr;
}

#endif /* MELEE_NATIVE */
