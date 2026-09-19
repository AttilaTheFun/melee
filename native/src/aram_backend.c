/* Native backing for GameCube ARAM. Game-visible ARAM addresses are offsets;
 * only main-memory addresses expand to the host pointer width. */
#include "melee_aram.h"
#include <dolphin/ar.h>
#include <dolphin/os.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#define ARAM_SIZE (16u * 1024u * 1024u)
#define ARAM_BASE 0x4000u
static unsigned char* memory;
static u32* lengths;
static u32 capacity, allocated, top;
static ARQCallback callback;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready = PTHREAD_COND_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static int started, busy;
static struct { u32 type, offset, size; ARAddress host; } dma;
static void require(int condition, char* reason)
{ if (!condition) OSPanic(__FILE__, __LINE__, "%s", reason); }
static void* worker(void* unused)
{
    (void)unused;
    for (;;) {
        pthread_mutex_lock(&lock);
        while (!busy) pthread_cond_wait(&ready, &lock);
        pthread_mutex_unlock(&lock);
        int level = OSDisableInterrupts();
        pthread_mutex_lock(&lock);
        if (dma.type == ARAM_DIR_MRAM_TO_ARAM) memcpy(memory + dma.offset, (void*)dma.host, dma.size);
        else memcpy((void*)dma.host, memory + dma.offset, dma.size);
        ARQCallback notify = callback;
        busy = 0;
        pthread_mutex_unlock(&lock);
        if (notify) notify(NULL);
        OSRestoreInterrupts(level);
    }
    return NULL;
}
static void start_worker(void)
{
    pthread_t thread;
    if (!pthread_create(&thread, NULL, worker, NULL)) { pthread_detach(thread); started = 1; }
}
u32 ARInit(u32* stack, u32 count)
{
    pthread_once(&once, start_worker);
    int level = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    require(started, "ARAM worker unavailable");
    if (!memory) {
        require(stack || !count, "ARAM allocation stack is missing");
        memory = calloc(1, ARAM_SIZE); require(memory != NULL, "ARAM backing allocation failed");
        lengths = stack; capacity = count; allocated = 0; top = ARAM_BASE; callback = NULL;
    }
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(level); return ARAM_BASE;
}
void ARReset(void)
{
    int level = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    require(!busy, "Cannot reset ARAM during DMA");
    free(memory); memory = NULL; lengths = NULL; capacity = allocated = 0; callback = NULL;
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(level);
}
int ARCheckInit(void) { pthread_mutex_lock(&lock); int result = memory != NULL; pthread_mutex_unlock(&lock); return result; }
u32 ARGetBaseAddress(void) { return ARAM_BASE; }
u32 ARGetSize(void) { pthread_mutex_lock(&lock); u32 result = memory ? ARAM_SIZE : 0; pthread_mutex_unlock(&lock); return result; }
void ARSetSize(void) {} /* Original SDK entry point is also a no-op. */
u32 ARAlloc(u32 length)
{
    int level = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    require(memory && !(length & 31) && allocated < capacity && length <= ARAM_SIZE - top,
            "Invalid ARAM allocation or exhausted arena");
    u32 result = top; top += length; lengths[allocated++] = length;
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(level); return result;
}
u32 ARFree(u32* length)
{
    int level = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    require(memory && allocated, "ARAM allocation stack is empty");
    u32 size = lengths[--allocated]; top -= size; if (length) *length = size;
    u32 result = top; pthread_mutex_unlock(&lock); OSRestoreInterrupts(level); return result;
}
ARQCallback ARRegisterDMACallback(ARQCallback cb)
{
    int level = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    ARQCallback previous = callback; callback = cb;
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(level); return previous;
}
u32 ARGetDMAStatus(void) { pthread_mutex_lock(&lock); u32 result = busy ? 0x200 : 0; pthread_mutex_unlock(&lock); return result; }
void ARStartDMA(u32 type, ARAddress mainmem_addr, u32 aram_addr, u32 length)
{
    int level = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    if (!(memory && !busy && type <= 1 && mainmem_addr && !(mainmem_addr & 31) &&
          !(aram_addr & 31) && !(length & 31) && aram_addr <= ARAM_SIZE && length <= ARAM_SIZE - aram_addr))
        OSPanic(__FILE__, __LINE__, "Invalid ARAM DMA: initialized=%d busy=%d type=%u host=%p offset=%u length=%u",
                memory!=NULL,busy,type,(void*)mainmem_addr,aram_addr,length);
    dma.type = type; dma.host = mainmem_addr; dma.offset = aram_addr; dma.size = length;
    busy = 1; pthread_cond_signal(&ready);
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(level);
}

bool melee_aram_read(uint32_t offset, void* destination, size_t bytes)
{
    int level = OSDisableInterrupts();
    pthread_mutex_lock(&lock);
    bool valid = memory && offset <= ARAM_SIZE && bytes <= ARAM_SIZE - offset &&
                 (destination || bytes == 0);
    if (valid && bytes) memcpy(destination, memory + offset, bytes);
    pthread_mutex_unlock(&lock);
    OSRestoreInterrupts(level);
    return valid;
}
