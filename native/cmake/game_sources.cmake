# Shared source manifest for the native and WebAssembly full-game builds.
file(GLOB_RECURSE full_game_sources CONFIGURE_DEPENDS
  "${MELEE_ROOT}/src/melee/*.c" "${MELEE_ROOT}/src/sysdolphin/*.c")
file(GLOB native_backend_sources CONFIGURE_DEPENDS "${MELEE_ROOT}/native/src/*.c")
list(APPEND native_backend_sources "${MELEE_ROOT}/native/src/thp_decode.cpp")
# gx_pixel.c is an isolated CPU register-capture test backend. Aurora owns
# these GX symbols in the real graphics build.
list(REMOVE_ITEM native_backend_sources "${MELEE_ROOT}/native/src/gx_pixel.c")
set(full_game_sdk_sources
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/ax/AXAlloc.c"
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/ax/AXAux.c"
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/ax/AXVPB.c"
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/axfx/axfx.c"
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/axfx/delay.c"
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/axfx/chorus.c"
  "${MELEE_ROOT}/src/MSL/float.c"
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/ar/arq.c"
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/os/OSArena.c"
  "${MELEE_ROOT}/extern/dolphin/src/dolphin/os/OSTime.c")
foreach(module mtx vec mtxvec mtx44)
  list(APPEND full_game_sdk_sources "${MELEE_ROOT}/extern/dolphin/src/dolphin/mtx/${module}.c")
endforeach()
