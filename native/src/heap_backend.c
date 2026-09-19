/* Caller-owned arena heaps for the native game. Metadata uses host pointers;
 * payloads keep the SDK's 32-byte alignment. No GameCube addresses are mapped. */
#include <dolphin/os/OSAlloc.h>
#include <dolphin/os.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct NativeBlock {
    size_t bytes; /* Includes the block header. */
    struct NativeBlock* prev;
    struct NativeBlock* next;
    unsigned allocated;
    unsigned reserved;
} NativeBlock;
_Static_assert(sizeof(NativeBlock) == 32, "Heap header must preserve alignment");
typedef struct {
    uintptr_t start, end;
    NativeBlock* first;
} NativeHeap;
static NativeHeap* heaps;
static int heap_count;
static uintptr_t arena_low, arena_high;
static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_int game_heap = -1;
volatile OSHeapHandle __OSCurrHeap = -1;

static uintptr_t align_down(uintptr_t p) { return p & ~(uintptr_t)31; }
static uintptr_t align_up(uintptr_t p) { return (p + 31) & ~(uintptr_t)31; }
static int valid(int h) { return h >= 0 && h < heap_count && heaps[h].first; }
static void invalid(char* reason) { OSPanic(__FILE__, __LINE__, "%s", reason); }

void* OSInitAlloc(void* start, void* end, int max_heaps)
{
    uintptr_t low = (uintptr_t)start, high = (uintptr_t)end;
    if (max_heaps <= 0 || low > UINTPTR_MAX - 31 || high <= low) return NULL;
    low = align_up(low); high = align_down(high);
    if (high <= low || high - low < 64) return NULL;
    NativeHeap* fresh = calloc((size_t)max_heaps, sizeof(*fresh));
    if (!fresh) return NULL;
    pthread_mutex_lock(&heap_lock);
    /* Reinitialization invalidates old handles, as in the original SDK. */
    free(heaps); heaps = fresh; heap_count = max_heaps;
    arena_low = low; arena_high = high; __OSCurrHeap = -1;
    atomic_store(&game_heap, -1);
    pthread_mutex_unlock(&heap_lock);
    return (void*)low;
}

int OSCreateHeap(void* start, void* end)
{
    uintptr_t low = (uintptr_t)start, high = (uintptr_t)end;
    if (low > UINTPTR_MAX - 31) return -1;
    low = align_up(low); high = align_down(high);
    pthread_mutex_lock(&heap_lock);
    int slot = -1;
    if (!heaps || low < arena_low || high > arena_high || high <= low || high - low < 64) goto done;
    for (int i = 0; i < heap_count; ++i) {
        if (!heaps[i].first) { if (slot < 0) slot = i; }
        else if (low < heaps[i].end && high > heaps[i].start) { slot = -1; goto done; }
    }
    if (slot >= 0) {
        NativeBlock* block = (NativeBlock*)low;
        *block = (NativeBlock){.bytes = high - low};
        heaps[slot] = (NativeHeap){low, high, block};
    }
done:
    pthread_mutex_unlock(&heap_lock);
    return slot;
}

void* OSAllocFromHeap(int h, unsigned long size)
{
    if (!size || size > SIZE_MAX - 63) return NULL;
    size_t required = align_up(size) + sizeof(NativeBlock);
    pthread_mutex_lock(&heap_lock);
    if (!valid(h)) invalid("OSAllocFromHeap: invalid heap");
    void* result = NULL;
    for (NativeBlock* b = heaps[h].first; b; b = b->next) {
        if (b->allocated || b->bytes < required) continue;
        if (b->bytes - required >= 64) {
            NativeBlock* rest = (NativeBlock*)((unsigned char*)b + required);
            *rest = (NativeBlock){.bytes = b->bytes - required, .prev = b, .next = b->next};
            if (rest->next) rest->next->prev = rest;
            b->next = rest; b->bytes = required;
        }
        b->allocated = 1;
        result = b + 1;
        break;
    }
    pthread_mutex_unlock(&heap_lock);
    return result;
}

void OSFreeToHeap(int h, void* ptr)
{
    pthread_mutex_lock(&heap_lock);
    if (!valid(h)) invalid("OSFreeToHeap: invalid heap");
    NativeBlock* b;
    for (b = heaps[h].first; b && (void*)(b + 1) != ptr; b = b->next) {}
    if (!b || !b->allocated) invalid("OSFreeToHeap: pointer not allocated by this heap");
    b->allocated = 0;
    if (b->next && !b->next->allocated) {
        NativeBlock* next = b->next;
        b->bytes += next->bytes; b->next = next->next;
        if (b->next) b->next->prev = b;
    }
    if (b->prev && !b->prev->allocated) {
        NativeBlock* previous = b->prev;
        previous->bytes += b->bytes; previous->next = b->next;
        if (previous->next) previous->next->prev = previous;
    }
    pthread_mutex_unlock(&heap_lock);
}

long OSCheckHeap(int h)
{
    pthread_mutex_lock(&heap_lock);
    long result = -1;
    if (!valid(h)) goto done;
    uintptr_t cursor = heaps[h].start;
    NativeBlock* previous = NULL;
    size_t available = 0;
    for (NativeBlock* b = heaps[h].first; b; b = b->next) {
        if ((uintptr_t)b != cursor || cursor > heaps[h].end || heaps[h].end - cursor < sizeof(*b) ||
            b->prev != previous || b->bytes < 64 || (b->bytes & 31) ||
            b->bytes > heaps[h].end - cursor || b->allocated > 1) goto done;
        if (!b->allocated) available += b->bytes - sizeof(*b);
        cursor += b->bytes; previous = b;
    }
    if (cursor == heaps[h].end && available <= LONG_MAX) result = (long)available;
done:
    pthread_mutex_unlock(&heap_lock);
    return result;
}

int OSSetCurrentHeap(int h)
{
    pthread_mutex_lock(&heap_lock);
    if (!valid(h)) invalid("OSSetCurrentHeap: invalid heap");
    int previous = __OSCurrHeap; __OSCurrHeap = h;
    pthread_mutex_unlock(&heap_lock);
    return previous;
}

void OSDestroyHeap(int h)
{
    pthread_mutex_lock(&heap_lock);
    if (!valid(h)) invalid("OSDestroyHeap: invalid heap");
    heaps[h] = (NativeHeap){0};
    if (__OSCurrHeap == h) __OSCurrHeap = -1;
    if (atomic_load(&game_heap) == h) atomic_store(&game_heap, -1);
    pthread_mutex_unlock(&heap_lock);
}

unsigned long OSReferentSize(void* ptr)
{
    pthread_mutex_lock(&heap_lock);
    unsigned long result = 0;
    for (int h = 0; h < heap_count; ++h)
        for (NativeBlock* b = heaps[h].first; b; b = b->next)
            if (b->allocated && (void*)(b+1) == ptr) result = b->bytes - sizeof(*b);
    pthread_mutex_unlock(&heap_lock);
    return result;
}

/* HSD selects its game heap independently of the SDK's current-heap macro. */
OSHeapHandle HSD_GetHeap(void) { return atomic_load(&game_heap); }
void HSD_SetHeap(OSHeapHandle h)
{
    pthread_mutex_lock(&heap_lock);
    if (!valid(h)) invalid("HSD_SetHeap: invalid heap");
    atomic_store(&game_heap, h);
    pthread_mutex_unlock(&heap_lock);
}
