#!/usr/bin/env python3
"""Copy the pinned Emdawn implementation with thread-safe JS handle cleanup.

Never mutates Emscripten's shared cache. The replacement object is linked before
Emscripten's port archive; its JS imports and public headers are still the port's.
"""
from pathlib import Path
import hashlib
import subprocess
import sys

em = Path(sys.argv[1])
out = Path(sys.argv[2])
out.parent.mkdir(parents=True, exist_ok=True)
source = em/'cache/ports/emdawnwebgpu/emdawnwebgpu_pkg/webgpu/src/webgpu.cpp'
if not source.exists():
    subprocess.run([str(em/'emcc'), '--use-port=emdawnwebgpu', '-pthread', '-x', 'c', '-c', '-',
                    '-o', str(out.parent/'port-bootstrap.o')], input='', text=True, check=True)
original = source.read_bytes()
if hashlib.sha256(original).hexdigest() != 'fdb0072f46dc2d86b99383ea849fad408b768c2b9d19bf9f2defbaa41389587b':
    raise SystemExit('Unexpected Emdawn source. Review the lifetime patch before changing the pinned toolchain.')
s = original.decode()
s = s.replace('  RefCounted() = default;', '''  RefCounted() = default;

  // Browser bridge: destructors may still use their JS object, but another
  // worker must not reuse this address until main-thread deletion completes.
  // Class deallocation runs after the complete destructor and before free.
  static void operator delete(void* object) noexcept {
    emwgpuDelete(object);
    ::operator delete(object);
  }''')
s = s.replace('''      // emwgpuDelete() removes the pointer from the jsObjects mapping.
      // Considering some class implementation may need to use it in
      // destructor, we call emwgpuDelete() after the pointer delete.
      // This also applies to implementation of `wgpu{Type}Release`.
      emwgpuDelete(value);''', '''      // RefCounted::operator delete removes the JS entry before freeing memory.''')
s = s.replace('      emwgpuDelete(o);                           \\\n', '')
assert s.count('emwgpuDelete(') == 2, 'Only the declaration and deallocator should remain'
if not out.exists() or out.read_text() != s:
    out.write_text(s)
