cmake_minimum_required(VERSION 3.25)
project(MeleeAuroraProbe LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(MELEE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(MELEE_AURORA_SOURCE "${MELEE_ROOT}/native/build/deps/aurora" CACHE PATH "Pinned Aurora checkout")
if(NOT EXISTS "${MELEE_AURORA_SOURCE}/CMakeLists.txt")
  message(FATAL_ERROR "Run native/tools/prepare_aurora.py first")
endif()
execute_process(COMMAND git -C "${MELEE_AURORA_SOURCE}" rev-parse HEAD
                OUTPUT_VARIABLE aurora_revision OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
if(NOT aurora_revision STREQUAL "749d6ee7a22bdfab78c8ece9047bca5d79aa72ca")
  message(FATAL_ERROR "The GX ABI adapter requires the pinned Aurora revision")
endif()
execute_process(COMMAND git -C "${MELEE_AURORA_SOURCE}" diff --binary
                OUTPUT_VARIABLE aurora_diff COMMAND_ERROR_IS_FATAL ANY)
file(READ "${CMAKE_CURRENT_LIST_DIR}/aurora.patch" aurora_patch)
if(NOT aurora_diff STREQUAL aurora_patch)
  message(FATAL_ERROR "Run native/tools/prepare_aurora.py to apply the reviewed backend fixes")
endif()
set(AURORA_ENABLE_DVD OFF CACHE BOOL "Use the existing native disc backend" FORCE)
set(AURORA_ENABLE_CARD OFF CACHE BOOL "No CARD integration in this probe" FORCE)
set(AURORA_ENABLE_THP OFF CACHE BOOL "No THP integration in this probe" FORCE)
set(AURORA_ENABLE_RMLUI OFF CACHE BOOL "No UI in this probe" FORCE)
add_subdirectory("${MELEE_AURORA_SOURCE}" aurora EXCLUDE_FROM_ALL)

# Separate translation units intentionally use the two SDK header sets. Merely
# compiling the game with Aurora's headers would hide an ABI mismatch.
add_library(gx_abi_client OBJECT gx_abi_client.c)
target_compile_definitions(gx_abi_client PRIVATE MELEE_NATIVE MELEE_AURORA)
target_include_directories(gx_abi_client PRIVATE "${MELEE_ROOT}/extern/dolphin/include")
add_library(gx_abi_reference OBJECT gx_abi_reference.cpp)
target_link_libraries(gx_abi_reference PRIVATE aurora::gx)
target_include_directories(gx_abi_reference PRIVATE "${MELEE_AURORA_SOURCE}")
add_library(gx_fog_client OBJECT gx_fog.c gx_modes.c)
target_compile_definitions(gx_fog_client PRIVATE MELEE_NATIVE MELEE_AURORA)
target_include_directories(gx_fog_client PRIVATE "${MELEE_ROOT}/extern/dolphin/include")
add_library(melee_vi_client OBJECT
  "${MELEE_ROOT}/native/src/vi_backend.c"
  "${MELEE_ROOT}/native/src/os_interrupt.c")
target_compile_definitions(melee_vi_client PRIVATE MELEE_NATIVE)
target_include_directories(melee_vi_client PRIVATE "${MELEE_ROOT}/native/include"
  "${MELEE_ROOT}/extern/dolphin/include")
add_library(melee_aurora_compat STATIC $<TARGET_OBJECTS:melee_vi_client> gx_getters.cpp gx_misc.cpp xfb.cpp $<TARGET_OBJECTS:gx_fog_client>)
target_include_directories(melee_aurora_compat PRIVATE "${MELEE_AURORA_SOURCE}")
target_link_libraries(melee_aurora_compat PUBLIC aurora::gx aurora::vi)
target_link_libraries(melee_aurora_compat PRIVATE absl::flat_hash_map)
add_executable(gx_abi_probe $<TARGET_OBJECTS:gx_abi_client> $<TARGET_OBJECTS:gx_abi_reference>)
target_link_libraries(gx_abi_probe PRIVATE melee_aurora_compat)

enable_testing()
add_test(NAME gx_abi COMMAND gx_abi_probe)
add_library(gx_misc_client OBJECT gx_misc_probe.c)
target_compile_definitions(gx_misc_client PRIVATE MELEE_NATIVE MELEE_AURORA)
target_include_directories(gx_misc_client PRIVATE "${MELEE_ROOT}/extern/dolphin/include")
add_executable(gx_misc_probe $<TARGET_OBJECTS:gx_misc_client> $<TARGET_OBJECTS:gx_abi_reference>)
target_link_libraries(gx_misc_probe PRIVATE melee_aurora_compat)
add_test(NAME gx_misc COMMAND gx_misc_probe)
add_executable(vi_probe vi_probe.cpp)
target_include_directories(vi_probe PRIVATE "${MELEE_AURORA_SOURCE}")
target_link_libraries(vi_probe PRIVATE melee_aurora_compat)
add_test(NAME vi_configure COMMAND vi_probe)

# Game SDK headers stay isolated from Aurora headers in C translation units.
set(hsd_modules debug class object hash id memory objalloc list mtx fobj aobj jobj
    dobj pobj mobj tobj robj spline displayfunc state util bytecode cobj wobj
    lobj fog tev texp texpdag perf random video)
set(hsd_sources "${MELEE_ROOT}/src/sysdolphin/baselib/psdisptev.c" hsd_video_gpu_probe.c hsd_scene_probe.c "${MELEE_ROOT}/src/melee/it/kinds/itpeachturnip.c" fighter_selection.c stock_probe.c training_probe.c
    "${MELEE_ROOT}/src/MSL/float.c"
    "${MELEE_ROOT}/src/melee/gm/gm_1884.c"
    "${MELEE_ROOT}/src/melee/pl/player.c")
foreach(module IN LISTS hsd_modules)
  list(APPEND hsd_sources "${MELEE_ROOT}/src/sysdolphin/baselib/${module}.c")
endforeach()
foreach(module sis_bank heap_backend os_interrupt diagnostics runtime_backend mtx_backend cache_backend scene scene_desc title environment menu hud character_select effects particle_bank player_params stage_models dynamic_model stage_collision stage_params onett_params item_scripts onett_stage battle_stage item_model item_animation item_foods item_wstar item_mushroom item_yoyo item_special item_article item_attributes item_hurtbones item_dynamics item_colors items_data item_common hand_data fox_data ness_data mario_data captain_data wireframe_data sandbag_data mars_data purin_data donkey_data koopa_data luigi_data pikachu_data yoshi_data zelda_data sheik_data link_data samus_data peach_data mewtwo_data iceclimbers_data gamewatch_data kirby_data kirby_copy kirby_composite_copy character_attributes character_aux character_collision character_dynamics character_motions character_part_anims character_parts character_sounds character_wait hand_items fox_items ness_items mario_items koopa_items luigi_items pikachu_items yoshi_items zelda_items peach_items kirby_items mewtwo_items iceclimbers_items gamewatch_items sheik_items link_items samus_items character_models fighter_command fighter_models fighter_common fighter_parts fighter_shakes fighter_modifiers fighter_aux fighter_cpu fighter_data camera lights archive joint material texture vertex animation visibility action)
  list(APPEND hsd_sources "${MELEE_ROOT}/native/src/${module}.c")
endforeach()
foreach(module mtx vec mtxvec mtx44)
  list(APPEND hsd_sources "${MELEE_ROOT}/extern/dolphin/src/dolphin/mtx/${module}.c")
endforeach()
list(APPEND hsd_sources "${MELEE_ROOT}/src/melee/lb/lbanim.c" "${MELEE_ROOT}/src/sysdolphin/baselib/archive.c")
list(APPEND hsd_sources "${MELEE_ROOT}/src/melee/lb/lbcommand.c"
  "${MELEE_ROOT}/src/melee/lb/lbspdisplay.c"
  "${MELEE_ROOT}/src/melee/ft/kinds/ftCommon/ftCo_09F4.c"
  "${MELEE_ROOT}/src/melee/if/ifstatus.c"
  "${MELEE_ROOT}/src/melee/gr/granime.c"
  "${MELEE_ROOT}/src/melee/ft/ftaction.c" "${MELEE_ROOT}/src/melee/ft/ftparts.c"
  "${MELEE_ROOT}/src/melee/ft/ftmaterial.c" "${MELEE_ROOT}/src/melee/ft/fighter.c"
  "${MELEE_ROOT}/src/melee/ft/ftcommon.c")
foreach(fighter Mario Fox Kirby Peach GameWatch)
  string(TOLOWER "${fighter}" lower)
  list(APPEND hsd_sources "${MELEE_ROOT}/src/melee/ft/kinds/ft${fighter}/ft${lower}.c")
endforeach()
add_library(hsd_scene_client OBJECT ${hsd_sources})
target_compile_definitions(hsd_scene_client PRIVATE MELEE_NATIVE MELEE_AURORA)
target_include_directories(hsd_scene_client PRIVATE "${MELEE_ROOT}/src"
  "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/extern/dolphin/include")
add_executable(hsd_scene_probe $<TARGET_OBJECTS:hsd_scene_client> $<TARGET_OBJECTS:gx_abi_reference>)
target_link_libraries(hsd_scene_probe PRIVATE melee_aurora_compat)
target_link_options(hsd_scene_probe PRIVATE -Wl,-dead_strip)

if(EXISTS "${MELEE_ROOT}/native/build/MnMaAll.dat")
  add_test(NAME hsd_menu_archive COMMAND hsd_scene_probe --menu-archive "${MELEE_ROOT}/native/build/MnMaAll.dat")
  set_tests_properties(hsd_menu_archive PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
endif()
if(EXISTS "${MELEE_ROOT}/native/build/PlCo.dat")
  add_test(NAME hsd_fighter_data COMMAND hsd_scene_probe --fighter-data "${MELEE_ROOT}/native/build/PlCo.dat")
  set_tests_properties(hsd_fighter_data PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
  add_test(NAME hsd_fighter_models COMMAND hsd_scene_probe --fighter-models "${MELEE_ROOT}/native/build/PlCo.dat")
  set_tests_properties(hsd_fighter_models PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
endif()
foreach(character Fx Fc Pe Kb Gw Ns)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat")
    add_test(NAME hsd_character_models_${character} COMMAND hsd_scene_probe --character-models "${MELEE_ROOT}/native/build/Pl${character}.dat")
  endif()
endforeach()

foreach(extension dat usd)
  if(EXISTS "${MELEE_ROOT}/native/build/ItCo.${extension}")
    add_test(NAME hsd_item_models_${extension} COMMAND hsd_scene_probe --item-models "${MELEE_ROOT}/native/build/ItCo.${extension}")
    add_test(NAME hsd_item_animations_${extension} COMMAND hsd_scene_probe --item-animations "${MELEE_ROOT}/native/build/ItCo.${extension}")
    add_test(NAME hsd_item_foods_${extension} COMMAND hsd_scene_probe --item-foods "${MELEE_ROOT}/native/build/ItCo.${extension}")
    add_test(NAME hsd_item_wstar_${extension} COMMAND hsd_scene_probe --item-wstar "${MELEE_ROOT}/native/build/ItCo.${extension}")
    add_test(NAME hsd_item_mushrooms_${extension} COMMAND hsd_scene_probe --item-mushrooms "${MELEE_ROOT}/native/build/ItCo.${extension}")
    add_test(NAME hsd_item_articles_${extension} COMMAND hsd_scene_probe --item-articles "${MELEE_ROOT}/native/build/ItCo.${extension}")
    add_test(NAME hsd_items_data_${extension} COMMAND hsd_scene_probe --items-data "${MELEE_ROOT}/native/build/ItCo.${extension}")
  endif()
  if(EXISTS "${MELEE_ROOT}/native/build/MnSlMap.${extension}")
    add_test(NAME hsd_stage_select_${extension} COMMAND hsd_scene_probe --stage-select "${MELEE_ROOT}/native/build/MnSlMap.${extension}")
  endif()
  if(EXISTS "${MELEE_ROOT}/native/build/MnSlChr.${extension}")
    add_test(NAME hsd_character_select_${extension} COMMAND hsd_scene_probe --character-select "${MELEE_ROOT}/native/build/MnSlChr.${extension}")
    add_test(NAME hsd_menu_extra_${extension} COMMAND hsd_scene_probe --menu-extra "${MELEE_ROOT}/native/build/MnExtAll.${extension}")
  endif()
endforeach()
if(EXISTS "${MELEE_ROOT}/native/build/GrNBa.dat")
  add_test(NAME hsd_battlefield_models COMMAND hsd_scene_probe --stage-models "${MELEE_ROOT}/native/build/GrNBa.dat")
endif()
if(EXISTS "${MELEE_ROOT}/native/build/GrOt.usd")
  add_test(NAME hsd_onett_archive COMMAND hsd_scene_probe --onett-archive "${MELEE_ROOT}/native/build/GrOt.usd")
  add_test(NAME hsd_onett_quake COMMAND hsd_scene_probe --dynamic-model "${MELEE_ROOT}/native/build/GrOt.usd" quake_model_set)
  add_test(NAME hsd_onett_models COMMAND hsd_scene_probe --stage-models "${MELEE_ROOT}/native/build/GrOt.usd")
endif()
add_test(NAME hsd_scene COMMAND hsd_scene_probe)
add_test(NAME hsd_stock COMMAND hsd_scene_probe --stock)
add_test(NAME hsd_training COMMAND hsd_scene_probe --training)
if(EXISTS "${MELEE_ROOT}/native/build/NtMsgWin.dat")
  add_test(NAME hsd_message_scene COMMAND hsd_scene_probe --message-scene "${MELEE_ROOT}/native/build/NtMsgWin.dat")
  set_tests_properties(hsd_message_scene PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
endif()
foreach(extension dat usd)
  set(title_scene "${MELEE_ROOT}/native/build/GmTtAll.${extension}")
  if(EXISTS "${title_scene}")
    add_test(NAME hsd_title_models_${extension} COMMAND hsd_scene_probe --title-models "${title_scene}")
    set_tests_properties(hsd_title_models_${extension} PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
  endif()
  set(card_scene "${MELEE_ROOT}/native/build/NtMemAc.${extension}")
  if(EXISTS "${card_scene}")
    add_test(NAME hsd_card_scene_${extension} COMMAND hsd_scene_probe --scene-desc "${card_scene}")
    set_tests_properties(hsd_card_scene_${extension} PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
  endif()
endforeach()

option(MELEE_AURORA_SANITIZE_HSD "Check native HSD object lifecycle" ON)
if(MELEE_AURORA_SANITIZE_HSD)
  target_compile_options(hsd_scene_client PRIVATE -fsanitize=address,undefined -g)
  target_link_options(hsd_scene_probe PRIVATE -fsanitize=address,undefined)
endif()

set_tests_properties(hsd_scene hsd_stock hsd_training PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")

# Owned game data is optional and never downloaded by configuration.
set(asset_codes Fx Mr Kb Pe Gw)
set(asset_names Fox Mario Kirby Peach Gamewatch)
foreach(code name IN ZIP_LISTS asset_codes asset_names)
  set(costume "${MELEE_ROOT}/native/build/Pl${code}Nr.dat")
  set(motion "${MELEE_ROOT}/native/build/motion-checks/Pl${code}AJ.dat")
  if(EXISTS "${costume}" AND EXISTS "${motion}")
    add_test(NAME hsd_asset_${code} COMMAND hsd_scene_probe "${costume}" "Ply${name}5K_Share_joint" "${motion}" --record)
    set_tests_properties(hsd_asset_${code} PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
    set(fighter "${MELEE_ROOT}/native/build/Pl${code}.dat")
    if(EXISTS "${fighter}")
      add_test(NAME hsd_selected_${code} COMMAND hsd_scene_probe "${costume}" "Ply${name}5K_Share_joint" "${motion}" --record --fighter "${fighter}" "ftData${name}")
      set_tests_properties(hsd_selected_${code} PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
    endif()
  endif()
endforeach()

add_library(melee_headless_gpu STATIC headless_gpu.cpp gpu_completion.cpp)
target_include_directories(melee_headless_gpu PRIVATE "${MELEE_AURORA_SOURCE}")
target_link_libraries(melee_headless_gpu PUBLIC melee_aurora_compat PNG::PNG PRIVATE absl::flat_hash_map)
target_link_libraries(hsd_scene_probe PRIVATE melee_headless_gpu)

# Full original-game link audit against the actual GX/VI backend. This target
# is excluded from normal probe builds and is never registered as a test:
# even a successful link would not make its unadapted entry point safe to run.
include("${MELEE_ROOT}/native/cmake/game_sources.cmake")
add_library(melee_full_game_objects OBJECT EXCLUDE_FROM_ALL
  ${full_game_sources} ${native_backend_sources} ${full_game_sdk_sources})
target_compile_definitions(melee_full_game_objects PRIVATE MELEE_NATIVE MELEE_AURORA)
target_include_directories(melee_full_game_objects PRIVATE "${MELEE_ROOT}/src"
  "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/extern/dolphin/include"
  "${MELEE_ROOT}/extern/dolphin/src" "${MELEE_ROOT}/native/build/generated")
add_executable(melee_game_link_audit EXCLUDE_FROM_ALL game_audit_entry.c $<TARGET_OBJECTS:melee_full_game_objects>)
target_link_libraries(melee_game_link_audit PRIVATE melee_aurora_compat)
target_link_options(melee_game_link_audit PRIVATE -Wl,-dead_strip)
add_executable(melee_game_startup EXCLUDE_FROM_ALL game_startup.cpp game_startup_input.c $<TARGET_OBJECTS:melee_full_game_objects>)
target_include_directories(melee_game_startup PRIVATE "${MELEE_AURORA_SOURCE}" "${MELEE_ROOT}/native/include")
set_source_files_properties(game_startup_input.c PROPERTIES COMPILE_DEFINITIONS MELEE_NATIVE INCLUDE_DIRECTORIES "${MELEE_ROOT}/extern/dolphin/include;${MELEE_ROOT}/src")
target_link_libraries(melee_game_startup PRIVATE melee_aurora_compat PNG::PNG absl::flat_hash_map)
target_link_options(melee_game_startup PRIVATE -Wl,-dead_strip)
option(MELEE_AURORA_SANITIZE_GAME "Instrument original game startup memory accesses" OFF)
add_executable(melee_robj_list_test EXCLUDE_FROM_ALL "${MELEE_ROOT}/native/tests/test_robj_list.c" $<TARGET_OBJECTS:melee_full_game_objects>)
target_compile_definitions(melee_robj_list_test PRIVATE MELEE_NATIVE)
target_include_directories(melee_robj_list_test PRIVATE "${MELEE_ROOT}/src" "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/extern/dolphin/include")
target_compile_options(melee_robj_list_test PRIVATE -UNDEBUG)
target_link_libraries(melee_robj_list_test PRIVATE melee_aurora_compat)
target_link_options(melee_robj_list_test PRIVATE -Wl,-dead_strip)
add_test(NAME robj_list_game_native COMMAND melee_robj_list_test)
if(MELEE_AURORA_SANITIZE_GAME)
  target_compile_options(melee_robj_list_test PRIVATE -fsanitize=address)
  target_link_options(melee_robj_list_test PRIVATE -fsanitize=address)
  target_compile_options(melee_full_game_objects PRIVATE -fsanitize=address -g -fno-omit-frame-pointer)
  target_link_options(melee_game_startup PRIVATE -fsanitize=address)
  target_link_options(melee_game_link_audit PRIVATE -fsanitize=address)
endif()


# Instrument original HSD state accesses while the real native GPU/VI callbacks
# execute on separate workers. The third-party renderer is not TSAN-instrumented.
add_library(hsd_scene_client_tsan OBJECT ${hsd_sources})
set_target_properties(hsd_scene_client_tsan PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_compile_definitions(hsd_scene_client_tsan PRIVATE MELEE_NATIVE MELEE_AURORA)
target_include_directories(hsd_scene_client_tsan PRIVATE "${MELEE_ROOT}/src"
  "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/extern/dolphin/include")
target_compile_options(hsd_scene_client_tsan PRIVATE -fsanitize=thread -g)
add_executable(hsd_scene_probe_tsan EXCLUDE_FROM_ALL
  $<TARGET_OBJECTS:hsd_scene_client_tsan> $<TARGET_OBJECTS:gx_abi_reference>)
target_link_libraries(hsd_scene_probe_tsan PRIVATE melee_headless_gpu)
target_link_options(hsd_scene_probe_tsan PRIVATE -Wl,-dead_strip -fsanitize=thread)

if(EXISTS "${MELEE_ROOT}/native/build/PlNs.dat")
  add_test(NAME hsd_ness_items COMMAND hsd_scene_probe --ness-items "${MELEE_ROOT}/native/build/PlNs.dat")
  if(EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlNsAJ.dat")
    add_test(NAME hsd_ness_data COMMAND hsd_scene_probe --ness-data "${MELEE_ROOT}/native/build/PlNs.dat" "${MELEE_ROOT}/native/build/motion-checks/PlNsAJ.dat")
  endif()
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfNsData.dat")
  add_test(NAME hsd_ness_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfNsData.dat" effNessDataTable)
endif()

foreach(extension dat usd)
  if(EXISTS "${MELEE_ROOT}/native/build/IfPrize.${extension}")
    add_test(NAME hsd_prize_${extension} COMMAND hsd_scene_probe --prize-scene "${MELEE_ROOT}/native/build/IfPrize.${extension}")
  endif()
  if(EXISTS "${MELEE_ROOT}/native/build/GmRst.${extension}")
    add_test(NAME hsd_results_${extension} COMMAND hsd_scene_probe --results-scenes "${MELEE_ROOT}/native/build/GmRst.${extension}")
  endif()
  if(EXISTS "${MELEE_ROOT}/native/build/GmPause.${extension}")
    add_test(NAME hsd_pause_${extension} COMMAND hsd_scene_probe --pause-scene "${MELEE_ROOT}/native/build/GmPause.${extension}")
  endif()
endforeach()

foreach(extension dat usd)
  if(EXISTS "${MELEE_ROOT}/native/build/IfAll.${extension}")
    add_test(NAME hsd_hud_${extension} COMMAND hsd_scene_probe --hud "${MELEE_ROOT}/native/build/IfAll.${extension}")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/IfCoGet.dat")
  add_test(NAME hsd_coget_scene COMMAND hsd_scene_probe --coget-scene "${MELEE_ROOT}/native/build/IfCoGet.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrOt.usd")
  add_test(NAME hsd_onett_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrOt.usd")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfCoData.dat")
  add_test(NAME hsd_common_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfCoData.dat" effCommonDataTable)
endif()

# App-facing runtime: UI owns presentation and publishes controller snapshots;
# the dedicated game thread owns the original loop and GX frame submission.
add_library(melee_game_runtime SHARED EXCLUDE_FROM_ALL game_runtime.cpp $<TARGET_OBJECTS:melee_full_game_objects>)
target_include_directories(melee_game_runtime PRIVATE "${MELEE_AURORA_SOURCE}" "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/src")
target_link_libraries(melee_game_runtime PRIVATE melee_aurora_compat absl::flat_hash_map)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  add_library(melee_runtime_probe_state OBJECT runtime_probe_state.c)
  target_compile_definitions(melee_runtime_probe_state PRIVATE MELEE_NATIVE)
  target_include_directories(melee_runtime_probe_state PRIVATE "${MELEE_ROOT}/src" "${MELEE_ROOT}/extern/dolphin/include")
  target_sources(melee_game_runtime PRIVATE $<TARGET_OBJECTS:melee_runtime_probe_state>)
  target_compile_definitions(melee_game_runtime PRIVATE MELEE_RUNTIME_PROBES)
endif()
target_link_options(melee_game_runtime PRIVATE -Wl,-dead_strip "-Wl,-exported_symbol,_melee_*")
if(MELEE_AURORA_SANITIZE_GAME)
  target_link_options(melee_game_runtime PRIVATE -fsanitize=address)
endif()
add_executable(melee_runtime_probe EXCLUDE_FROM_ALL runtime_probe.c)
add_executable(melee_hud_death_test EXCLUDE_FROM_ALL "${MELEE_ROOT}/native/tests/test_hud_death.c")
add_executable(melee_match_end_layout_test EXCLUDE_FROM_ALL "${MELEE_ROOT}/native/tests/test_match_end_layout.c")
target_compile_definitions(melee_match_end_layout_test PRIVATE MELEE_NATIVE)
target_include_directories(melee_match_end_layout_test PRIVATE "${MELEE_ROOT}/src" "${MELEE_ROOT}/extern/dolphin/include")
target_compile_options(melee_match_end_layout_test PRIVATE -UNDEBUG)
add_test(NAME match_end_layout_native COMMAND melee_match_end_layout_test)
target_compile_definitions(melee_hud_death_test PRIVATE MELEE_NATIVE)
target_include_directories(melee_hud_death_test PRIVATE "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/src" "${MELEE_ROOT}/extern/dolphin/include")
target_link_libraries(melee_hud_death_test PRIVATE melee_game_runtime)
target_compile_options(melee_hud_death_test PRIVATE -UNDEBUG)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  target_compile_options(melee_hud_death_test PRIVATE -fsanitize=address)
  target_link_options(melee_hud_death_test PRIVATE -fsanitize=address)
endif()
add_test(NAME hud_death_native COMMAND melee_hud_death_test)
add_executable(melee_damage_result_test EXCLUDE_FROM_ALL "${MELEE_ROOT}/native/tests/test_damage_result.c")
target_compile_definitions(melee_damage_result_test PRIVATE MELEE_NATIVE)
target_include_directories(melee_damage_result_test PRIVATE "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/src" "${MELEE_ROOT}/extern/dolphin/include")
target_compile_options(melee_damage_result_test PRIVATE -UNDEBUG)
add_test(NAME damage_result_native COMMAND melee_damage_result_test)
target_compile_definitions(melee_runtime_probe PRIVATE MELEE_NATIVE)
target_include_directories(melee_runtime_probe PRIVATE "${MELEE_ROOT}/native/include" "${MELEE_ROOT}/src" "${MELEE_ROOT}/extern/dolphin/include")
target_link_libraries(melee_runtime_probe PRIVATE melee_game_runtime PNG::PNG)
target_compile_options(melee_runtime_probe PRIVATE -UNDEBUG)
if(MELEE_AURORA_SANITIZE_GAME)
  target_compile_options(melee_runtime_probe PRIVATE -fsanitize=address)
  target_link_options(melee_runtime_probe PRIVATE -fsanitize=address)
endif()

foreach(character Fx Fc Ns)
  if(character STREQUAL "Fx")
    set(result_character Fox)
  elseif(character STREQUAL "Fc")
    set(result_character Falco)
  else()
    set(result_character Ness)
  endif()
  if(EXISTS "${MELEE_ROOT}/native/build/GmRstM${character}.dat")
    add_test(NAME hsd_results_motions_${character} COMMAND hsd_scene_probe --results-motions
      "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/GmRstM${character}.dat"
      "ftData${result_character}" "ftDemoResultMotionFile${result_character}")
  endif()
endforeach()

foreach(character Mr Dr)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat")
    add_test(NAME hsd_mario_items_${character} COMMAND hsd_scene_probe --mario-items "${MELEE_ROOT}/native/build/Pl${character}.dat")
  endif()
endforeach()

foreach(character Mr Dr)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat" AND EXISTS "${MELEE_ROOT}/native/build/motion-checks/Pl${character}AJ.dat")
    add_test(NAME hsd_mario_data_${character} COMMAND hsd_scene_probe --mario-data "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/motion-checks/Pl${character}AJ.dat")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/EfMrData.dat")
  add_test(NAME hsd_mario_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfMrData.dat" effMarioDataTable)
endif()

foreach(character Mr Dr)
  if(character STREQUAL "Mr")
    set(result_character Mario)
    set(result_count 16)
  else()
    set(result_character Drmario)
    set(result_count 14)
  endif()
  if(EXISTS "${MELEE_ROOT}/native/build/GmRstM${character}.dat")
    add_test(NAME hsd_results_motions_${character} COMMAND hsd_scene_probe --results-motions
      "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/GmRstM${character}.dat"
      "ftData${result_character}" "ftDemoResultMotionFile${result_character}" "${result_count}")
  endif()
endforeach()

foreach(character Ca Gn)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat" AND EXISTS "${MELEE_ROOT}/native/build/motion-checks/Pl${character}AJ.dat")
    add_test(NAME hsd_captain_data_${character} COMMAND hsd_scene_probe --captain-data "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/motion-checks/Pl${character}AJ.dat")
  endif()
endforeach()

foreach(character Ca Gn)
  if(character STREQUAL "Ca")
    set(effect_character Captain)
  else()
    set(effect_character Ganon)
  endif()
  if(EXISTS "${MELEE_ROOT}/native/build/Ef${character}Data.dat")
    add_test(NAME hsd_captain_effects_${character} COMMAND hsd_scene_probe --effects
      "${MELEE_ROOT}/native/build/Ef${character}Data.dat" "eff${effect_character}DataTable")
  endif()
endforeach()

foreach(character Ca Gn)
  if(character STREQUAL "Ca")
    set(result_character Captain)
  else()
    set(result_character Ganon)
  endif()
  if(EXISTS "${MELEE_ROOT}/native/build/GmRstM${character}.dat")
    add_test(NAME hsd_results_motions_${character} COMMAND hsd_scene_probe --results-motions
      "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/GmRstM${character}.dat"
      "ftData${result_character}" "ftDemoResultMotionFile${result_character}")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/PlDk.dat" AND EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlDkAJ.dat")
  add_test(NAME hsd_donkey_data COMMAND hsd_scene_probe --donkey-data "${MELEE_ROOT}/native/build/PlDk.dat" "${MELEE_ROOT}/native/build/motion-checks/PlDkAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfDkData.dat")
  add_test(NAME hsd_donkey_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfDkData.dat" effDonkeyDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMDk.dat")
  add_test(NAME hsd_results_motions_Dk COMMAND hsd_scene_probe --results-motions
    "${MELEE_ROOT}/native/build/PlDk.dat" "${MELEE_ROOT}/native/build/GmRstMDk.dat"
    ftDataDonkey ftDemoResultMotionFileDonkey)
endif()

foreach(character Kp Gk)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat")
    add_test(NAME hsd_koopa_items_${character} COMMAND hsd_scene_probe --koopa-items "${MELEE_ROOT}/native/build/Pl${character}.dat")
  endif()
endforeach()

foreach(character Kp Gk)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat" AND EXISTS "${MELEE_ROOT}/native/build/motion-checks/Pl${character}AJ.dat")
    add_test(NAME hsd_koopa_data_${character} COMMAND hsd_scene_probe --koopa-data "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/motion-checks/Pl${character}AJ.dat")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/EfKpData.dat")
  add_test(NAME hsd_koopa_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfKpData.dat" effKoopaDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMKp.dat")
  add_test(NAME hsd_results_motions_Kp COMMAND hsd_scene_probe --results-motions
    "${MELEE_ROOT}/native/build/PlKp.dat" "${MELEE_ROOT}/native/build/GmRstMKp.dat"
    ftDataKoopa ftDemoResultMotionFileKoopa)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlLg.dat")
  add_test(NAME hsd_luigi_items COMMAND hsd_scene_probe --luigi-items "${MELEE_ROOT}/native/build/PlLg.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlLg.dat" AND EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlLgAJ.dat")
  add_test(NAME hsd_luigi_data COMMAND hsd_scene_probe --luigi-data "${MELEE_ROOT}/native/build/PlLg.dat" "${MELEE_ROOT}/native/build/motion-checks/PlLgAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfLgData.dat")
  add_test(NAME hsd_luigi_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfLgData.dat" effLuigiDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMLg.dat")
  add_test(NAME hsd_results_motions_Lg COMMAND hsd_scene_probe --results-motions
    "${MELEE_ROOT}/native/build/PlLg.dat" "${MELEE_ROOT}/native/build/GmRstMLg.dat"
    ftDataLuigi ftDemoResultMotionFileLuigi)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrNBa.dat")
  add_test(NAME hsd_battle_archive COMMAND hsd_scene_probe --battle-archive "${MELEE_ROOT}/native/build/GrNBa.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrNLa.dat")
  add_test(NAME hsd_final_empty_camera COMMAND hsd_scene_probe --stage-empty-camera "${MELEE_ROOT}/native/build/GrNLa.dat" 3)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrNLa.dat")
  add_test(NAME hsd_final_light_playback COMMAND hsd_scene_probe --stage-light-playback "${MELEE_ROOT}/native/build/GrNLa.dat" 3)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrNLa.dat")
  add_test(NAME hsd_final_stage_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrNLa.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrNLa.dat")
  add_test(NAME hsd_final_archive COMMAND hsd_scene_probe --final-archive "${MELEE_ROOT}/native/build/GrNLa.dat")
endif()

add_test(NAME hsd_particle_tev COMMAND hsd_scene_probe --particle-tev)

foreach(character Ms Fe)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat" AND EXISTS "${MELEE_ROOT}/native/build/Pl${character}AJ.dat")
    add_test(NAME hsd_mars_data_${character} COMMAND hsd_scene_probe --mars-data "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/Pl${character}AJ.dat")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/EfMsData.dat")
  add_test(NAME hsd_mars_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfMsData.dat" effMarsDataTable)
endif()
if(EXISTS "${MELEE_ROOT}/native/build/EfFeData.dat")
  add_test(NAME hsd_roy_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfFeData.dat" effEmblemDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMMs.dat")
  add_test(NAME hsd_results_motions_Ms COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlMs.dat" "${MELEE_ROOT}/native/build/GmRstMMs.dat" ftDataMars ftDemoResultMotionFileMars)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMFe.dat")
  add_test(NAME hsd_results_motions_Fe COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlFe.dat" "${MELEE_ROOT}/native/build/GmRstMFe.dat" ftDataEmblem ftDemoResultMotionFileEmblem)
endif()

foreach(character Pk Pc)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat")
    add_test(NAME hsd_pikachu_items_${character} COMMAND hsd_scene_probe --pikachu-items "${MELEE_ROOT}/native/build/Pl${character}.dat")
  endif()
endforeach()

foreach(character Pk Pc)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}AJ.dat")
    add_test(NAME hsd_pikachu_data_${character} COMMAND hsd_scene_probe --pikachu-data "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/Pl${character}AJ.dat")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/EfPkData.dat")
  add_test(NAME hsd_pikachu_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfPkData.dat" effPikachuDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMPk.dat")
  add_test(NAME hsd_results_motions_Pk COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlPk.dat" "${MELEE_ROOT}/native/build/GmRstMPk.dat" ftDataPikachu ftDemoResultMotionFilePikachu)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMPc.dat")
  add_test(NAME hsd_results_motions_Pc COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlPc.dat" "${MELEE_ROOT}/native/build/GmRstMPc.dat" ftDataPichu ftDemoResultMotionFilePichu)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlPrAJ.dat")
  add_test(NAME hsd_purin_data COMMAND hsd_scene_probe --purin-data "${MELEE_ROOT}/native/build/PlPr.dat" "${MELEE_ROOT}/native/build/PlPrAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfPrData.dat")
  add_test(NAME hsd_purin_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfPrData.dat" effPurinDataTable)
endif()
if(EXISTS "${MELEE_ROOT}/native/build/GmRstMPr.dat")
  add_test(NAME hsd_results_motions_Pr COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlPr.dat" "${MELEE_ROOT}/native/build/GmRstMPr.dat" ftDataPurin ftDemoResultMotionFilePurin)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlYs.dat")
  add_test(NAME hsd_yoshi_items COMMAND hsd_scene_probe --yoshi-items "${MELEE_ROOT}/native/build/PlYs.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlYsAJ.dat")
  add_test(NAME hsd_yoshi_data COMMAND hsd_scene_probe --yoshi-data "${MELEE_ROOT}/native/build/PlYs.dat" "${MELEE_ROOT}/native/build/PlYsAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfYsData.dat")
  add_test(NAME hsd_yoshi_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfYsData.dat" effYoshiDataTable)
endif()
if(EXISTS "${MELEE_ROOT}/native/build/GmRstMYs.dat")
  add_test(NAME hsd_results_motions_Ys COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlYs.dat" "${MELEE_ROOT}/native/build/GmRstMYs.dat" ftDataYoshi ftDemoResultMotionFileYoshi)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlZd.dat")
  add_test(NAME hsd_zelda_items COMMAND hsd_scene_probe --zelda-items "${MELEE_ROOT}/native/build/PlZd.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlZd.dat" AND EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlZdAJ.dat")
  add_test(NAME hsd_zelda_data COMMAND hsd_scene_probe --zelda-data "${MELEE_ROOT}/native/build/PlZd.dat" "${MELEE_ROOT}/native/build/motion-checks/PlZdAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlSk.dat")
  add_test(NAME hsd_sheik_items COMMAND hsd_scene_probe --sheik-items "${MELEE_ROOT}/native/build/PlSk.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlSk.dat" AND EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlSkAJ.dat")
  add_test(NAME hsd_sheik_data COMMAND hsd_scene_probe --sheik-data "${MELEE_ROOT}/native/build/PlSk.dat" "${MELEE_ROOT}/native/build/motion-checks/PlSkAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfZdData.dat")
  add_test(NAME hsd_zelda_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfZdData.dat" effZeldaDataTable)
endif()
if(EXISTS "${MELEE_ROOT}/native/build/GmRstMZd.dat")
  add_test(NAME hsd_results_motions_Zd COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlZd.dat" "${MELEE_ROOT}/native/build/GmRstMZd.dat" ftDataZelda ftDemoResultMotionFileZelda)
endif()
if(EXISTS "${MELEE_ROOT}/native/build/GmRstMSk.dat")
  add_test(NAME hsd_results_motions_Sk COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlSk.dat" "${MELEE_ROOT}/native/build/GmRstMSk.dat" ftDataSeak ftDemoResultMotionFileSeak)
endif()

foreach(character Lk Cl)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat")
    add_test(NAME hsd_link_basic_items_${character} COMMAND hsd_scene_probe --link-basic-items "${MELEE_ROOT}/native/build/Pl${character}.dat")
  endif()
endforeach()

foreach(character Lk Cl)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat")
    add_test(NAME hsd_link_tether_items_${character} COMMAND hsd_scene_probe --link-tether-items "${MELEE_ROOT}/native/build/Pl${character}.dat")
  endif()
endforeach()

foreach(character Lk Cl)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat")
    add_test(NAME hsd_link_items_${character} COMMAND hsd_scene_probe --link-items "${MELEE_ROOT}/native/build/Pl${character}.dat")
  endif()
endforeach()

foreach(character Lk Cl)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat" AND EXISTS "${MELEE_ROOT}/native/build/motion-checks/Pl${character}AJ.dat")
    add_test(NAME hsd_link_data_${character} COMMAND hsd_scene_probe --link-data "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/motion-checks/Pl${character}AJ.dat")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/PlSs.dat")
  if(EXISTS "${MELEE_ROOT}/native/build/GmRstMSs.dat")
    add_test(NAME hsd_results_motions_Ss COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlSs.dat" "${MELEE_ROOT}/native/build/GmRstMSs.dat" ftDataSamus ftDemoResultMotionFileSamus)
  endif()
  add_test(NAME hsd_samus_data COMMAND hsd_scene_probe --samus-data "${MELEE_ROOT}/native/build/PlSs.dat" "${MELEE_ROOT}/native/build/motion-checks/PlSsAJ.dat")
  add_test(NAME hsd_samus_items COMMAND hsd_scene_probe --samus-items "${MELEE_ROOT}/native/build/PlSs.dat")
  add_test(NAME hsd_samus_grapple COMMAND hsd_scene_probe --samus-grapple "${MELEE_ROOT}/native/build/PlSs.dat")
  add_test(NAME hsd_samus_basic_items COMMAND hsd_scene_probe --samus-basic-items "${MELEE_ROOT}/native/build/PlSs.dat")
endif()
if(EXISTS "${MELEE_ROOT}/native/build/EfLkData.dat")
  add_test(NAME hsd_link_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfLkData.dat" effLinkDataTable)
endif()
if(EXISTS "${MELEE_ROOT}/native/build/GmRstMLk.dat")
  add_test(NAME hsd_results_motions_Lk COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlLk.dat" "${MELEE_ROOT}/native/build/GmRstMLk.dat" ftDataLink ftDemoResultMotionFileLink)
endif()
if(EXISTS "${MELEE_ROOT}/native/build/GmRstMCl.dat")
  add_test(NAME hsd_results_motions_Cl COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlCl.dat" "${MELEE_ROOT}/native/build/GmRstMCl.dat" ftDataClink ftDemoResultMotionFileClink)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlPe.dat")
  add_test(NAME hsd_peach_items COMMAND hsd_scene_probe --peach-items "${MELEE_ROOT}/native/build/PlPe.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlPeAJ.dat")
  add_test(NAME hsd_peach_data COMMAND hsd_scene_probe --peach-data "${MELEE_ROOT}/native/build/PlPe.dat" "${MELEE_ROOT}/native/build/motion-checks/PlPeAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfPeData.dat")
  add_test(NAME hsd_peach_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfPeData.dat" effPeachDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMPe.dat")
  add_test(NAME hsd_results_motions_Pe COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlPe.dat" "${MELEE_ROOT}/native/build/GmRstMPe.dat" ftDataPeach ftDemoResultMotionFilePeach)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlMt.dat")
  add_test(NAME hsd_mewtwo_items COMMAND hsd_scene_probe --mewtwo-items "${MELEE_ROOT}/native/build/PlMt.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlMtAJ.dat")
  add_test(NAME hsd_mewtwo_data COMMAND hsd_scene_probe --mewtwo-data "${MELEE_ROOT}/native/build/PlMt.dat" "${MELEE_ROOT}/native/build/motion-checks/PlMtAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfMtData.dat")
  add_test(NAME hsd_mewtwo_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfMtData.dat" effMewtwoDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMMt.dat")
  add_test(NAME hsd_results_motions_Mt COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlMt.dat" "${MELEE_ROOT}/native/build/GmRstMMt.dat" ftDataMewtwo ftDemoResultMotionFileMewtwo)
endif()

foreach(code Pp Nn)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${code}.dat")
    add_test(NAME hsd_iceclimbers_items_${code} COMMAND hsd_scene_probe --iceclimbers-items "${MELEE_ROOT}/native/build/Pl${code}.dat")
  endif()
endforeach()

foreach(code Pp Nn)
  if(EXISTS "${MELEE_ROOT}/native/build/motion-checks/Pl${code}AJ.dat")
    add_test(NAME hsd_iceclimbers_data_${code} COMMAND hsd_scene_probe --iceclimbers-data "${MELEE_ROOT}/native/build/Pl${code}.dat" "${MELEE_ROOT}/native/build/motion-checks/Pl${code}AJ.dat")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/EfIcData.dat")
  add_test(NAME hsd_iceclimbers_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfIcData.dat" effIceclimberDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMPn.dat")
  add_test(NAME hsd_results_motions_Pp COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlPp.dat" "${MELEE_ROOT}/native/build/GmRstMPn.dat" ftDataPopo ftDemoResultMotionFilePopo)
  add_test(NAME hsd_results_motions_Nn COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlNn.dat" "${MELEE_ROOT}/native/build/GmRstMPn.dat" ftDataNana ftDemoResultMotionFileNana)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlGw.dat")
  add_test(NAME hsd_gamewatch_items COMMAND hsd_scene_probe --gamewatch-items "${MELEE_ROOT}/native/build/PlGw.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlGwAJ.dat")
  add_test(NAME hsd_gamewatch_data COMMAND hsd_scene_probe --gamewatch-data "${MELEE_ROOT}/native/build/PlGw.dat" "${MELEE_ROOT}/native/build/motion-checks/PlGwAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMGw.dat")
  add_test(NAME hsd_results_motions_Gw COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlGw.dat" "${MELEE_ROOT}/native/build/GmRstMGw.dat" ftDataGamewatch ftDemoResultMotionFileGamewatch)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlKb.dat")
  add_test(NAME hsd_kirby_items COMMAND hsd_scene_probe --kirby-items "${MELEE_ROOT}/native/build/PlKb.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/motion-checks/PlKbAJ.dat")
  add_test(NAME hsd_kirby_data COMMAND hsd_scene_probe --kirby-data "${MELEE_ROOT}/native/build/PlKb.dat" "${MELEE_ROOT}/native/build/motion-checks/PlKbAJ.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/PlKbCpNs.dat")
  add_test(NAME hsd_kirby_copy_ness COMMAND hsd_scene_probe --kirby-copy-ness "${MELEE_ROOT}/native/build/PlKbCpNs.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GmRstMKb.dat")
  add_test(NAME hsd_results_motions_Kb COMMAND hsd_scene_probe --results-motions "${MELEE_ROOT}/native/build/PlKb.dat" "${MELEE_ROOT}/native/build/GmRstMKb.dat" ftDataKirby ftDemoResultMotionFileKirby)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpMr.dat")
  add_test(NAME hsd_kirby_copy_fireballs COMMAND hsd_scene_probe --kirby-copy-fireballs "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpMr.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpDr.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpLg.dat")
endif()

add_test(NAME kirby_copy_cache_reset COMMAND melee_game_startup --kirby-cache-reset)

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpFx.dat")
  add_test(NAME hsd_kirby_copy_fox COMMAND hsd_scene_probe --kirby-copy-fox "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpFx.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/EfKbFx.dat")
  add_test(NAME hsd_kirby_fox_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/EfKbFx.dat" effKirbyFoxDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpFc.dat")
  add_test(NAME hsd_kirby_falco_copy COMMAND hsd_scene_probe --kirby-falco-copy "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpFc.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbNrCpFc.dat")
  add_test(NAME hsd_kirby_falco_costumes COMMAND hsd_scene_probe --kirby-falco-costumes "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpFc.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbNrCpFc.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbYeCpFc.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbBuCpFc.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbReCpFc.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbGrCpFc.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbWhCpFc.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbNrCpDk.dat")
  add_test(NAME hsd_kirby_donkey_costumes COMMAND hsd_scene_probe --kirby-donkey-costumes "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpDk.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbNrCpDk.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbYeCpDk.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbBuCpDk.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbReCpDk.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbGrCpDk.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbWhCpDk.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbDk.dat")
  add_test(NAME hsd_kirby_donkey_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbDk.dat" effKirbyDonkeyDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpCa.dat")
  add_test(NAME hsd_kirby_copy_captain COMMAND hsd_scene_probe --kirby-copy-captain "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpCa.dat")
  add_test(NAME hsd_kirby_captain_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbCa.dat" effKirbyCaptainDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpGn.dat")
  add_test(NAME hsd_kirby_copy_ganon COMMAND hsd_scene_probe --kirby-copy-ganon "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpGn.dat")
  add_test(NAME hsd_kirby_ganon_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbGn.dat" effKirbyGanonDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpMs.dat")
  add_test(NAME hsd_kirby_copy_swords COMMAND hsd_scene_probe --kirby-copy-swords "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpMs.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpFe.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbMs.dat")
  add_test(NAME hsd_kirby_marth_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbMs.dat" effKirbyMarsDataTable)
  add_test(NAME hsd_kirby_roy_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbFe.dat" effKirbyEmblemDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpZd.dat")
  add_test(NAME hsd_kirby_copy_zelda COMMAND hsd_scene_probe --kirby-copy-zelda "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpZd.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbZd.dat")
  add_test(NAME hsd_kirby_zelda_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbZd.dat" effKirbyZeldaDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpSk.dat")
  add_test(NAME hsd_kirby_copy_sheik COMMAND hsd_scene_probe --kirby-copy-sheik "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpSk.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPe.dat")
  add_test(NAME hsd_kirby_copy_peach COMMAND hsd_scene_probe --kirby-copy-peach "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPe.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPp.dat")
  add_test(NAME hsd_kirby_copy_ice COMMAND hsd_scene_probe --kirby-copy-ice "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPp.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbIc.dat")
  add_test(NAME hsd_kirby_ice_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbIc.dat" effKirbyIceDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpSs.dat")
  add_test(NAME hsd_kirby_copy_samus COMMAND hsd_scene_probe --kirby-copy-samus "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpSs.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbSs.dat")
  add_test(NAME hsd_kirby_samus_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbSs.dat" effKirbySamusDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPk.dat" AND EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPc.dat")
  add_test(NAME hsd_kirby_copy_electric COMMAND hsd_scene_probe --kirby-copy-electric "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPk.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPc.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbPk.dat")
  add_test(NAME hsd_kirby_electric_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbPk.dat" effKirbyPikachuDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpKp.dat")
  add_test(NAME hsd_kirby_copy_koopa COMMAND hsd_scene_probe --kirby-copy-koopa "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpKp.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbKp.dat")
  add_test(NAME hsd_kirby_koopa_effects COMMAND hsd_scene_probe --effects "${MELEE_ROOT}/native/build/kirby-copy-archives/EfKbKp.dat" effKirbyKoopaDataTable)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbWhCpMt.dat")
  add_test(NAME hsd_kirby_mewtwo_costumes COMMAND hsd_scene_probe --kirby-mewtwo-costumes "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpMt.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbNrCpMt.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbYeCpMt.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbBuCpMt.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbReCpMt.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbGrCpMt.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbWhCpMt.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpLk.dat" AND EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpCl.dat")
  add_test(NAME hsd_kirby_copy_links COMMAND hsd_scene_probe --kirby-copy-links "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpLk.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpCl.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbWhCpPr.dat")
  add_test(NAME hsd_kirby_purin_costumes COMMAND hsd_scene_probe --kirby-purin-costumes "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpPr.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbNrCpPr.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbYeCpPr.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbBuCpPr.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbReCpPr.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbGrCpPr.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbWhCpPr.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpYs.dat")
  add_test(NAME hsd_kirby_copy_yoshi COMMAND hsd_scene_probe --kirby-copy-yoshi "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpYs.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpGw.dat")
  add_test(NAME hsd_kirby_copy_chef_articles COMMAND hsd_scene_probe --kirby-chef-articles "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpGw.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbNrCpGw.dat")
  add_test(NAME hsd_kirby_gamewatch_costumes COMMAND hsd_scene_probe --kirby-gamewatch-costumes "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbCpGw.dat" "${MELEE_ROOT}/native/build/kirby-copy-archives/PlKbNrCpGw.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrOp.dat")
  add_test(NAME hsd_dreamland_archive COMMAND hsd_scene_probe --dreamland-archive "${MELEE_ROOT}/native/build/GrOp.dat")
  add_test(NAME hsd_dreamland_stage_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrOp.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrIz.dat")
  add_test(NAME hsd_fountain_archive COMMAND hsd_scene_probe --fountain-archive "${MELEE_ROOT}/native/build/GrIz.dat")
  add_test(NAME hsd_fountain_stage_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrIz.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrIz.dat")
  add_test(NAME hsd_fountain_light_colors_1 COMMAND hsd_scene_probe --stage-light-colors "${MELEE_ROOT}/native/build/GrIz.dat" 1)
  add_test(NAME hsd_fountain_light_colors_3 COMMAND hsd_scene_probe --stage-light-colors "${MELEE_ROOT}/native/build/GrIz.dat" 3)
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrSt.dat")
  add_test(NAME hsd_story_archive COMMAND hsd_scene_probe --story-archive "${MELEE_ROOT}/native/build/GrSt.dat")
  add_test(NAME hsd_story_stage_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrSt.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrPs.usd")
  add_test(NAME hsd_stadium_archive COMMAND hsd_scene_probe --stadium-archive "${MELEE_ROOT}/native/build/GrPs.usd")
  add_test(NAME hsd_stadium_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrPs.usd")
endif()
foreach(form RANGE 1 4)
  if(EXISTS "${MELEE_ROOT}/native/build/GrPs${form}.dat")
    add_test(NAME hsd_stadium_transform_${form} COMMAND hsd_scene_probe --stadium-transform-archive "${MELEE_ROOT}/native/build/GrPs${form}.dat")
    add_test(NAME hsd_stadium_playback_${form} COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrPs${form}.dat")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/GrGb.dat")
  add_test(NAME hsd_greatbay_archive COMMAND hsd_scene_probe --greatbay-archive "${MELEE_ROOT}/native/build/GrGb.dat")
  add_test(NAME hsd_greatbay_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrGb.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrKg.dat")
  add_test(NAME hsd_kongo_archive COMMAND hsd_scene_probe --kongo-archive "${MELEE_ROOT}/native/build/GrKg.dat")
  add_test(NAME hsd_kongo_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrKg.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrGd.dat")
  add_test(NAME hsd_japes_archive COMMAND hsd_scene_probe --japes-archive "${MELEE_ROOT}/native/build/GrGd.dat")
  add_test(NAME hsd_japes_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrGd.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrZe.dat")
  add_test(NAME hsd_brinstar_archive COMMAND hsd_scene_probe --brinstar-archive "${MELEE_ROOT}/native/build/GrZe.dat")
  add_test(NAME hsd_brinstar_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrZe.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrCs.dat")
  add_test(NAME hsd_castle_archive COMMAND hsd_scene_probe --castle-archive "${MELEE_ROOT}/native/build/GrCs.dat")
  add_test(NAME hsd_castle_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrCs.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrYt.dat")
  add_test(NAME hsd_yorster_archive COMMAND hsd_scene_probe --yorster-archive "${MELEE_ROOT}/native/build/GrYt.dat")
  add_test(NAME hsd_yorster_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrYt.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrGr.dat")
  add_test(NAME hsd_greens_archive COMMAND hsd_scene_probe --greens-archive "${MELEE_ROOT}/native/build/GrGr.dat")
  add_test(NAME hsd_greens_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrGr.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrSh.dat")
  add_test(NAME hsd_shrine_archive COMMAND hsd_scene_probe --shrine-archive "${MELEE_ROOT}/native/build/GrSh.dat")
  add_test(NAME hsd_shrine_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrSh.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrVe.usd")
  add_test(NAME hsd_venom_archive COMMAND hsd_scene_probe --venom-archive "${MELEE_ROOT}/native/build/GrVe.usd")
  add_test(NAME hsd_venom_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrVe.usd")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrCn.usd")
  add_test(NAME hsd_corneria_archive COMMAND hsd_scene_probe --corneria-archive "${MELEE_ROOT}/native/build/GrCn.usd")
  add_test(NAME hsd_corneria_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrCn.usd")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrMc.dat")
  add_test(NAME hsd_mutecity_archive COMMAND hsd_scene_probe --mutecity-archive "${MELEE_ROOT}/native/build/GrMc.dat")
  add_test(NAME hsd_mutecity_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrMc.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrBb.dat")
  add_test(NAME hsd_bigblue_archive COMMAND hsd_scene_probe --bigblue-archive "${MELEE_ROOT}/native/build/GrBb.dat")
  add_test(NAME hsd_bigblue_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrBb.dat")
endif()

add_test(NAME native_bigblue_car_lanes COMMAND melee_game_startup --bigblue-car-lanes)

if(EXISTS "${MELEE_ROOT}/native/build/GrFs.dat")
  add_test(NAME hsd_fourside_archive COMMAND hsd_scene_probe --fourside-archive "${MELEE_ROOT}/native/build/GrFs.dat")
  add_test(NAME hsd_fourside_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrFs.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrIm.dat")
  add_test(NAME hsd_whitebear_article COMMAND hsd_scene_probe --whitebear-article "${MELEE_ROOT}/native/build/GrIm.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrIm.dat")
  add_test(NAME hsd_icemt_archive COMMAND hsd_scene_probe --icemt-archive "${MELEE_ROOT}/native/build/GrIm.dat")
  add_test(NAME hsd_icemt_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrIm.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrFz.dat")
  add_test(NAME hsd_tools_article COMMAND hsd_scene_probe --tools-article "${MELEE_ROOT}/native/build/GrFz.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrFz.dat")
  add_test(NAME hsd_flatzone_archive COMMAND hsd_scene_probe --flatzone-archive "${MELEE_ROOT}/native/build/GrFz.dat")
  add_test(NAME hsd_flatzone_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrFz.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrKr.dat")
  add_test(NAME hsd_kraid_archive COMMAND hsd_scene_probe --kraid-archive "${MELEE_ROOT}/native/build/GrKr.dat")
  add_test(NAME hsd_kraid_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrKr.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrRc.dat")
  add_test(NAME hsd_rcruise_archive COMMAND hsd_scene_probe --rcruise-archive "${MELEE_ROOT}/native/build/GrRc.dat")
  add_test(NAME hsd_rcruise_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrRc.dat")
endif()

if(EXISTS "${MELEE_ROOT}/native/build/GrPu.dat")
  add_test(NAME hsd_pura_archive COMMAND hsd_scene_probe --pura-archive "${MELEE_ROOT}/native/build/GrPu.dat")
  add_test(NAME hsd_pura_playback COMMAND hsd_scene_probe --stage-playback "${MELEE_ROOT}/native/build/GrPu.dat")
endif()

add_test(NAME native_pura_toon COMMAND melee_game_startup --pura-toon)

foreach(character Bo Gl)
  if(EXISTS "${MELEE_ROOT}/native/build/Pl${character}.dat" AND EXISTS "${MELEE_ROOT}/native/build/Pl${character}AJ.dat")
    add_test(NAME hsd_wireframe_data_${character} COMMAND hsd_scene_probe --wireframe-data "${MELEE_ROOT}/native/build/Pl${character}.dat" "${MELEE_ROOT}/native/build/Pl${character}AJ.dat")
  endif()
endforeach()

add_test(NAME native_ottosea_link COMMAND melee_game_startup --ottosea-link)

add_test(NAME native_icicle_contact COMMAND melee_game_startup --icicle-contact)

if(EXISTS "${MELEE_ROOT}/native/build/GrHe.dat")
  add_test(NAME hsd_heal_archive COMMAND hsd_scene_probe --heal-archive "${MELEE_ROOT}/native/build/GrHe.dat")
endif()

add_test(NAME native_stage_contact COMMAND melee_game_startup --stage-contact)

add_test(NAME native_allstar_roster COMMAND melee_game_startup --allstar-roster)

add_test(NAME hsd_texture_index COMMAND hsd_scene_probe --texture-index)
foreach(extension dat usd)
  if(EXISTS "${MELEE_ROOT}/native/build/GmTrain.${extension}")
    add_test(NAME hsd_training_archive_${extension} COMMAND hsd_scene_probe --training-archive "${MELEE_ROOT}/native/build/GmTrain.${extension}")
  endif()
endforeach()

foreach(character Ca Cl Dk Dr Fc Fe Fx Gn Gw Ic Kb Kp Lg Lk Mr Ms Mt Ns Pc Pe Pk Pr Sk Ss Ys Zd)
  if(EXISTS "${MELEE_ROOT}/native/build/GrT${character}.dat")
    add_test(NAME hsd_target_archive_${character} COMMAND hsd_scene_probe --target-archive "${MELEE_ROOT}/native/build/GrT${character}.dat")
  endif()
endforeach()

foreach(stage 1 2)
  if(EXISTS "${MELEE_ROOT}/native/build/GrI${stage}.dat")
    add_test(NAME hsd_inishie${stage}_archive COMMAND hsd_scene_probe --inishie${stage}-archive "${MELEE_ROOT}/native/build/GrI${stage}.dat")
  endif()
endforeach()

if(EXISTS "${MELEE_ROOT}/native/build/PlSb.dat")
  add_test(NAME hsd_sandbag_data COMMAND hsd_scene_probe --sandbag-data "${MELEE_ROOT}/native/build/PlSb.dat" "${MELEE_ROOT}/native/build/PlSbAJ.dat")
endif()
foreach(extension dat usd)
  if(EXISTS "${MELEE_ROOT}/native/build/GrHr.${extension}")
    add_test(NAME hsd_homerun_archive_${extension} COMMAND hsd_scene_probe --homerun-archive "${MELEE_ROOT}/native/build/GrHr.${extension}")
  endif()
endforeach()

foreach(file IfHrNoCn.dat IfHrNoCn.usd IfHrReco.dat IfHrReco.usd)
  if(EXISTS "${MELEE_ROOT}/native/build/${file}")
    add_test(NAME hsd_homerun_hud_${file} COMMAND hsd_scene_probe --homerun-hud "${MELEE_ROOT}/native/build/${file}")
  endif()
endforeach()

add_test(NAME native_player_mapping COMMAND melee_game_startup --player-mapping)
