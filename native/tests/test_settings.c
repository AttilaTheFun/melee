#include "melee_settings.h"
#include <dolphin/os.h>
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void expected_failure(int signal_number)
{
    (void) signal_number;
    _exit(86);
}

static void* update(void* context)
{
    (void) context;
    for (unsigned int i = 0; i < 20; ++i) {
        OSSetSoundMode(i);
        OSSetProgressiveMode(i + 1);
        assert(OSGetSoundMode() <= 1);
        assert(OSGetProgressiveMode() <= 1);
    }
    return NULL;
}

int main(int argc, char** argv)
{
    if (argc == 2) {
        assert(melee_settings_open(argv[1]));
        assert(OSGetSoundMode() == 0);
        assert(OSGetProgressiveMode() == 1);
        return 0;
    }
    char directory[] = "build/settings-XXXXXX";
    assert(mkdtemp(directory));
    char path[256], bad[256];
    snprintf(path, sizeof(path), "%s/system.bin", directory);
    snprintf(bad, sizeof(bad), "%s/bad.bin", directory);
    assert(!melee_settings_open(NULL));
    assert(!melee_settings_open(""));
    assert(melee_settings_open(path));
    assert(OSGetSoundMode() == 1 && OSGetProgressiveMode() == 0);
    OSSetSoundMode(0);
    OSSetProgressiveMode(1);
    pid_t child = fork();
    assert(child >= 0);
    if (!child) { execl(argv[0], argv[0], path, (char*) NULL); _exit(127); }
    int status;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status));
    unsigned char invalid[9] = {'M', 'S', 'R', 'M', 1, 0, 1, 0, 0};
    /* Reject truncation, trailing data, unknown versions and out-of-range bits.
     * A failed open must retain the currently configured file and values. */
    for (int variant = 0; variant < 5; ++variant) {
        int fd = open(bad, O_CREAT | O_TRUNC | O_WRONLY, 0600);
        assert(fd >= 0);
        size_t count = variant == 0 ? 7 : variant == 1 ? 9 : 8;
        invalid[4] = variant == 2 ? 2 : 1;
        invalid[5] = variant == 3 ? 2 : 0;
        invalid[7] = variant == 4 ? 1 : 0;
        assert(write(fd, invalid, count) == (ssize_t) count && close(fd) == 0);
        assert(!melee_settings_open(bad));
        assert(OSGetSoundMode() == 0 && OSGetProgressiveMode() == 1);
    }
    assert(!melee_settings_open(directory));
    pthread_t threads[4];
    for (int i = 0; i < 4; ++i) assert(!pthread_create(&threads[i], NULL, update, NULL));
    for (int i = 0; i < 4; ++i) assert(!pthread_join(threads[i], NULL));
    OSSetSoundMode(0xFFFFFFFEu);
    OSSetProgressiveMode(0xFFFFFFFFu);
    assert(melee_settings_open(path));
    assert(OSGetSoundMode() == 0 && OSGetProgressiveMode() == 1);
    /* A persistence failure must be reported instead of pretending the update
     * succeeded. Removing the parent path makes the write fail reliably. */
    char moved[256];
    snprintf(moved, sizeof(moved), "%s-moved", directory);
    assert(!rename(directory, moved));
    child = fork();
    assert(child >= 0);
    if (!child) {
        signal(SIGABRT, expected_failure);
        OSSetSoundMode(1);
        _exit(99);
    }
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 86);
    assert(!rename(moved, directory));
    assert(melee_settings_open(path));
    assert(OSGetSoundMode() == 0 && OSGetProgressiveMode() == 1);
    assert(!unlink(path) && !unlink(bad) && !rmdir(directory));
    puts("Native settings: persistent restart, atomic updates, corrupt-file rejection and concurrent access passed");
    return 0;
}
