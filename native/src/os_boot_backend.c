/* Native process startup arena. This is host RAM, not a mapping of GameCube
 * physical addresses. Its process lifetime matches the original OS arena. */
#include <dolphin/os.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define NATIVE_ARENA_BYTES (64u * 1024u * 1024u)
static pthread_once_t boot_once = PTHREAD_ONCE_INIT;
static void* game_memory;

static void initialize_host(void)
{
    if (posix_memalign(&game_memory, 32, NATIVE_ARENA_BYTES) != 0) {
        OSPanic(__FILE__, __LINE__, "Unable to allocate native game arena");
    }
    memset(game_memory, 0, NATIVE_ARENA_BYTES);
    OSSetArenaLo(game_memory);
    OSSetArenaHi((unsigned char*) game_memory + NATIVE_ARENA_BYTES);
    (void) OSGetTime();
}

void OSInit(void)
{
    pthread_once(&boot_once, initialize_host);
}

u32 OSGetPhysicalMemSize(void)
{
    OSInit();
    return NATIVE_ARENA_BYTES;
}

u32 OSGetConsoleSimulatedMemSize(void)
{
    return OSGetPhysicalMemSize();
}
