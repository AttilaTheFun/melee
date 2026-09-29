cmake_minimum_required(VERSION 3.25)
project(MeleeWebGame LANGUAGES C CXX)
if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "Configure with the pinned Emscripten emcmake tool")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
option(MELEE_WEB_ASAN "Instrument the diagnostic browser build with AddressSanitizer" OFF)
if(MELEE_WEB_ASAN)
  # Keep separate DWARF for diagnosis. Runtime source maps are expanded into
  # large JS object graphs and copied to every pthread during startup.
  add_compile_options(-O1 -g -fsanitize=address)
  add_link_options(-gseparate-dwarf -fsanitize=address)
endif()
get_filename_component(MELEE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${MELEE_ROOT}/native/cmake/game_sources.cmake")
add_library(melee_web_game_objects OBJECT ${full_game_sources} ${native_backend_sources} ${full_game_sdk_sources})
target_compile_definitions(melee_web_game_objects PRIVATE MELEE_NATIVE MELEE_AURORA)
target_compile_options(melee_web_game_objects PRIVATE -pthread)
target_include_directories(melee_web_game_objects PRIVATE "${MELEE_ROOT}/src"
  "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/extern/dolphin/include"
  "${MELEE_ROOT}/extern/dolphin/src")

option(MELEE_WEB_RENDERER "Build the browser Aurora renderer" OFF)
if(MELEE_WEB_RENDERER)
  set(AURORA_ENABLE_DVD OFF CACHE BOOL "" FORCE)
  set(AURORA_ENABLE_CARD OFF CACHE BOOL "" FORCE)
  set(AURORA_ENABLE_THP OFF CACHE BOOL "" FORCE)
  set(AURORA_ENABLE_RMLUI OFF CACHE BOOL "" FORCE)
  set(AURORA_CACHE_USE_ZSTD OFF CACHE BOOL "" FORCE)
  set(AURORA_SDL3_PROVIDER vendor CACHE STRING "" FORCE)
  set(AURORA_SDL3_LINKAGE static CACHE STRING "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  add_compile_options(-pthread)
  add_link_options(-pthread)
  add_subdirectory("${MELEE_ROOT}/native/build/deps/aurora" aurora EXCLUDE_FROM_ALL)
endif()

if(MELEE_WEB_RENDERER)
  add_executable(melee_gpu_bridge_probe gpu_bridge_probe.cpp)
  target_link_libraries(melee_gpu_bridge_probe PRIVATE dawn::webgpu_dawn)
  # Port libraries are added before custom libraries during Emscripten linking.
  target_link_options(melee_gpu_bridge_probe PRIVATE
    "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/webgpu_proxy.js"
    -sPROXY_TO_PTHREAD=1 -sPTHREAD_POOL_SIZE=4 -sEXIT_RUNTIME=1 -sASSERTIONS=2
    -sSTACK_SIZE=1048576)
  set_target_properties(melee_gpu_bridge_probe PROPERTIES SUFFIX ".js"
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/webgpu_proxy.js")
endif()

if(MELEE_WEB_RENDERER)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  get_filename_component(MELEE_EMSCRIPTEN_ROOT "${CMAKE_C_COMPILER}" DIRECTORY)
  set(MELEE_EMDAWN_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/emdawn-threadsafe/webgpu.cpp")
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/prepare_emdawn.py"
    "${MELEE_EMSCRIPTEN_ROOT}" "${MELEE_EMDAWN_SOURCE}" COMMAND_ERROR_IS_FATAL ANY)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/prepare_emdawn.py")
  add_library(melee_emdawn_threadsafe OBJECT "${MELEE_EMDAWN_SOURCE}")
  target_link_libraries(melee_emdawn_threadsafe PRIVATE dawn::webgpu_dawn)
  target_compile_options(melee_emdawn_threadsafe PRIVATE -UNDEBUG)
  target_sources(melee_gpu_bridge_probe PRIVATE $<TARGET_OBJECTS:melee_emdawn_threadsafe>)
  # Use an object target so all replacement C entry points are resolved before
  # the port archive is searched, including internal Ref<T> destruction.
  add_library(melee_browser_gpu STATIC browser_gpu.cpp)
  target_link_libraries(melee_browser_gpu PUBLIC melee_emdawn_threadsafe)
  target_sources(melee_browser_gpu INTERFACE $<TARGET_OBJECTS:melee_emdawn_threadsafe>)
  target_include_directories(melee_browser_gpu PRIVATE "${MELEE_ROOT}/native/build/deps/aurora")
  target_link_libraries(melee_browser_gpu PUBLIC aurora::gx aurora::vi)
endif()
if(MELEE_WEB_RENDERER)
  add_executable(melee_aurora_probe aurora_probe.cpp)
  target_include_directories(melee_aurora_probe PRIVATE "${MELEE_ROOT}/native/build/deps/aurora")
  target_link_libraries(melee_aurora_probe PRIVATE melee_browser_gpu absl::flat_hash_map)
  target_link_options(melee_aurora_probe PRIVATE
    "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/webgpu_proxy.js"
    -sPROXY_TO_PTHREAD=1 -sPTHREAD_POOL_SIZE=12 -sEXIT_RUNTIME=1 -sASSERTIONS=2
    -sSTACK_SIZE=2097152 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=268435456 -sMAXIMUM_MEMORY=2147483648)
  set_target_properties(melee_aurora_probe PROPERTIES SUFFIX ".js"
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/webgpu_proxy.js")
endif()

if(MELEE_WEB_RENDERER)
  add_library(melee_web_sdk OBJECT browser_input.c browser_save.c browser_state.c browser_net.c browser_fonts.c
    "${MELEE_ROOT}/native/aurora/gx_fog.c" "${MELEE_ROOT}/native/aurora/gx_modes.c")
  target_compile_definitions(melee_web_sdk PRIVATE MELEE_NATIVE MELEE_AURORA)
  target_include_directories(melee_web_sdk PRIVATE "${MELEE_ROOT}/src" "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/extern/dolphin/include")
  add_executable(melee_browser browser_runtime.cpp browser_disc.cpp
    "${MELEE_ROOT}/native/aurora/gx_getters.cpp" "${MELEE_ROOT}/native/aurora/gx_misc.cpp"
    "${MELEE_ROOT}/native/aurora/xfb.cpp" $<TARGET_OBJECTS:melee_web_sdk>
    $<TARGET_OBJECTS:melee_web_game_objects>)
  target_include_directories(melee_browser PRIVATE "${MELEE_ROOT}/native/build/deps/aurora" "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/native/aurora")
  target_link_libraries(melee_browser PRIVATE melee_browser_gpu absl::flat_hash_map)
  target_link_options(melee_browser PRIVATE
    --profiling-funcs --emit-symbol-map
    -lidbfs.js "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/persistence.js"
    "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/presentation.js"
    "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/input.js"
    "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/audio.js"
    "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/lifecycle.js"
    "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/net.js"
    "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/webgpu_proxy.js"
    -sPROXY_TO_PTHREAD=1 -sPTHREAD_POOL_SIZE=16 -sEXIT_RUNTIME=1 -sASSERTIONS=2
    -sSTACK_SIZE=4194304 -sDEFAULT_PTHREAD_STACK_SIZE=2097152
    -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=268435456 -sMAXIMUM_MEMORY=2147483648)
  if(NOT MELEE_WEB_ASAN)
    # Reserve low addresses for game ARAM offsets, never host objects.
    # ASan forbids a custom base and reserves a larger shadow region itself.
    target_link_options(melee_browser PRIVATE -sGLOBAL_BASE=16777216)
  else()
    target_sources(melee_browser PRIVATE browser_asan.c)
  endif()
  set_target_properties(melee_browser PROPERTIES SUFFIX ".js" LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/webgpu_proxy.js;${CMAKE_CURRENT_SOURCE_DIR}/presentation.js;${CMAKE_CURRENT_SOURCE_DIR}/input.js;${CMAKE_CURRENT_SOURCE_DIR}/audio.js;${CMAKE_CURRENT_SOURCE_DIR}/persistence.js;${CMAKE_CURRENT_SOURCE_DIR}/lifecycle.js;${CMAKE_CURRENT_SOURCE_DIR}/net.js")
endif()

configure_file(index.html index.html COPYONLY)
configure_file(audio-worklet.js audio-worklet.js COPYONLY)
