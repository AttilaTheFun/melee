/* Diagnostic-only: leave room for the game's large WebGPU staging mappings in
 * the 2 GiB Wasm address space. Keep a nonzero freed-allocation quarantine. */
const char* __asan_default_options(void)
{
    return "quarantine_size_mb=64";
}
