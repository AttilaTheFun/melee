/* The two game-visible SRAM preferences, stored separately from save data.
 * Explicit bytes keep this file independent of host endianness and C ABI. */
#include "melee_settings.h"
#include <dolphin/os.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static pthread_mutex_t settings_lock = PTHREAD_MUTEX_INITIALIZER;
static char* settings_path;
static unsigned char values[8] = {'M', 'S', 'R', 'M', 1, 1, 0, 0};

static bool write_settings(const char* path, const unsigned char data[8])
{
    size_t length = strlen(path);
    char* temporary = malloc(length + sizeof(".XXXXXX"));
    if (!temporary) return false;
    memcpy(temporary, path, length);
    memcpy(temporary + length, ".XXXXXX", sizeof(".XXXXXX"));
    int fd = mkstemp(temporary);
    bool ok = fd >= 0;
    size_t offset = 0;
    while (ok && offset < 8) {
        ssize_t count = write(fd, data + offset, 8 - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        offset += (size_t) count;
    }
    if (ok && fsync(fd) != 0) ok = false;
    if (fd >= 0 && close(fd) != 0) ok = false;
    if (ok && rename(temporary, path) != 0) ok = false;
    if (!ok && fd >= 0) unlink(temporary);
    free(temporary);
    return ok;
}

bool melee_settings_open(const char* path)
{
    if (!path || !*path) return false;
    char* copy = strdup(path);
    if (!copy) return false;
    unsigned char candidate[9] = {'M', 'S', 'R', 'M', 1, 1, 0, 0, 0};
    pthread_mutex_lock(&settings_lock);
    int fd = open(path, O_RDONLY);
    bool ok;
    if (fd < 0) {
        ok = errno == ENOENT && write_settings(path, candidate);
    } else {
        size_t offset = 0;
        ok = true;
        while (offset < sizeof(candidate)) {
            ssize_t count = read(fd, candidate + offset, sizeof(candidate) - offset);
            if (count < 0 && errno == EINTR) continue;
            if (count < 0) { ok = false; break; }
            if (count == 0) break;
            offset += (size_t) count;
        }
        if (close(fd) != 0) ok = false;
        ok = ok && offset == 8 && !memcmp(candidate, "MSRM\1", 5) &&
             candidate[5] <= 1 && candidate[6] <= 1 && candidate[7] == 0;
    }
    if (ok) {
        free(settings_path);
        settings_path = copy;
        memcpy(values, candidate, 8);
    } else {
        free(copy);
    }
    pthread_mutex_unlock(&settings_lock);
    return ok;
}

static u32 get_mode(unsigned int index)
{
    pthread_mutex_lock(&settings_lock);
    u32 result = values[index];
    pthread_mutex_unlock(&settings_lock);
    return result;
}

static void set_mode(unsigned int index, u32 mode)
{
    pthread_mutex_lock(&settings_lock);
    unsigned char candidate[8];
    memcpy(candidate, values, 8);
    candidate[index] = mode & 1; /* Original SRAM stores one bit per setting. */
    bool ok = settings_path && (candidate[index] == values[index] ||
                                write_settings(settings_path, candidate));
    if (ok) memcpy(values, candidate, 8);
    pthread_mutex_unlock(&settings_lock);
    if (!ok) OSPanic(__FILE__, __LINE__, "Unable to persist native system setting");
}

u32 OSGetSoundMode(void) { return get_mode(5); }
void OSSetSoundMode(u32 mode) { set_mode(5, mode); }
u32 OSGetProgressiveMode(void) { return get_mode(6); }
void OSSetProgressiveMode(u32 mode) { set_mode(6, mode); }
