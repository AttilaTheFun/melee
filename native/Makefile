CC = xcrun clang
CPPFLAGS = -DMELEE_NATIVE -Iinclude -I../extern/dolphin/include
CFLAGS = -std=c11 -Wall -Wextra -Werror -g
DIAGNOSTICS_SOURCES = src/diagnostics.c ../src/sysdolphin/baselib/debug.c
CORE = src/archive.c src/input.c
HSD_INPUT = src/pad_backend.c src/os_interrupt.c src/hsd_input.c
HSD_OBJECTS = build/controller.o build/rumble.o
MTX_SOURCES = ../extern/dolphin/src/dolphin/mtx/mtx.c ../extern/dolphin/src/dolphin/mtx/vec.c ../extern/dolphin/src/dolphin/mtx/mtxvec.c ../extern/dolphin/src/dolphin/mtx/mtx44.c
NATIVE_HEADERS = $(wildcard include/*.h) ../src/sysdolphin/baselib/forward.h ../src/sysdolphin/baselib/id.h ../src/sysdolphin/baselib/aobj.h ../src/sysdolphin/baselib/jobj.h ../src/Runtime/platform.h ../extern/dolphin/include/dolphin/types.h
NATIVE_HEADERS += ../extern/dolphin/include/dolphin/os.h
NATIVE_HEADERS += ../extern/dolphin/include/dolphin/ax.h
NATIVE_HEADERS += ../src/melee/lb/types.h ../src/melee/ft/types.h
NATIVE_HEADERS += ../extern/dolphin/include/dolphin/card.h $(wildcard ../extern/dolphin/include/dolphin/card/*.h)

build/inspect-archive build/test-native build/test-game-math build/test-hsd-input build/test-mtx: $(NATIVE_HEADERS)

.PHONY: all test audit clean
all: build/inspect-archive build/inspect-disc build/verify-disc
test: build/test-gx-pixel
test: build/test-visibility
test: build/test-command
test: build/test-action
ACTION_SOURCES = src/action.c src/fighter_command.c ../src/melee/lb/lbcommand.c ../src/melee/ft/ftaction.c
build/test-action: tests/test_action.c $(ACTION_SOURCES) src/archive.c $(NATIVE_HEADERS) ../src/melee/lb/types.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_action.c $(ACTION_SOURCES) src/archive.c -o $@
build/verify-action: tools/verify_action.c $(ACTION_SOURCES) src/archive.c $(NATIVE_HEADERS) ../src/melee/lb/types.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tools/verify_action.c $(ACTION_SOURCES) src/archive.c -o $@
build/test-command: tests/test_command.c ../src/melee/lb/lbcommand.c ../src/melee/lb/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_command.c ../src/melee/lb/lbcommand.c -o $@
VISIBILITY_SOURCES = src/visibility.c ../src/melee/ft/ftparts.c ../src/sysdolphin/baselib/dobj.c ../src/melee/ft/ftaction.c
VISIBILITY_SOURCES += ../src/melee/ft/kinds/ftMario/ftmario.c ../src/melee/ft/kinds/ftFox/ftfox.c ../src/melee/ft/kinds/ftPeach/ftpeach.c ../src/melee/ft/kinds/ftGameWatch/ftgamewatch.c ../src/melee/ft/kinds/ftKirby/ftkirby.c
build/test-visibility: tests/test_visibility.c $(VISIBILITY_SOURCES) src/action.c src/fighter_command.c ../src/melee/lb/lbcommand.c src/archive.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_visibility.c $(VISIBILITY_SOURCES) src/action.c src/fighter_command.c ../src/melee/lb/lbcommand.c src/archive.c $(DIAGNOSTICS_SOURCES) -o $@
build/verify-visibility: tools/verify_visibility.c $(VISIBILITY_SOURCES) src/archive.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tools/verify_visibility.c $(VISIBILITY_SOURCES) src/archive.c $(DIAGNOSTICS_SOURCES) -o $@

build/test-gx-pixel: tests/test_gx_pixel.c src/gx_pixel.c src/os_interrupt.c ../src/sysdolphin/baselib/state.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_gx_pixel.c src/gx_pixel.c src/os_interrupt.c ../src/sysdolphin/baselib/state.c -o $@

build:
	mkdir -p build

build/inspect-archive: tools/inspect_archive.c src/archive.c include/melee_archive.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/inspect_archive.c src/archive.c -o $@

build/test-native: tests/test_native.c $(CORE) include/melee_archive.h include/melee_input.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tests/test_native.c $(CORE) -o $@

build/test-game-math: tests/test_game_math.c ../src/melee/lb/lbvector.c ../src/placeholder.h ../src/Runtime/platform.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -Wl,-dead_strip tests/test_game_math.c ../src/melee/lb/lbvector.c -o $@

build/controller.o: ../src/sysdolphin/baselib/controller.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -c $< -o $@

build/rumble.o: ../src/sysdolphin/baselib/rumble.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -c $< -o $@

build/test-hsd-input: tests/test_hsd_input.c src/input.c $(HSD_INPUT) $(HSD_OBJECTS) $(DIAGNOSTICS_SOURCES) include/melee_pad_backend.h include/melee_hsd_input.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -I../src -fsanitize=address,undefined tests/test_hsd_input.c src/input.c $(HSD_INPUT) $(HSD_OBJECTS) $(DIAGNOSTICS_SOURCES) -o $@

build/test-mtx: tests/test_mtx.c src/mtx_backend.c $(MTX_SOURCES) ../src/melee/lb/lbvector.c | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_mtx.c src/mtx_backend.c $(MTX_SOURCES) ../src/melee/lb/lbvector.c -o $@

build/test-game-abi: tests/test_game_abi.c src/input.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -I../src -fsanitize=address,undefined tests/test_game_abi.c src/input.c -o $@

build/test-effect-params: tests/test_effect_params.c ../src/melee/ef/eflib.c ../src/melee/ef/eflib.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_effect_params.c ../src/melee/ef/eflib.c -o $@

build/test-animation-search: tests/test_animation_search.c ../src/melee/gr/granime.c ../src/melee/gr/granime.h ../src/sysdolphin/baselib/aobj.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_animation_search.c ../src/melee/gr/granime.c ../src/sysdolphin/baselib/aobj.c $(DIAGNOSTICS_SOURCES) -o $@

build/test-particle-sort: tests/test_particle_sort.c ../src/sysdolphin/baselib/psdisp.c ../src/sysdolphin/baselib/particle.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_particle_sort.c ../src/sysdolphin/baselib/psdisp.c ../src/sysdolphin/baselib/particle.c $(DIAGNOSTICS_SOURCES) -o $@

HEAP_SOURCES = src/heap_backend.c $(DIAGNOSTICS_SOURCES) ../src/sysdolphin/baselib/memory.c ../src/sysdolphin/baselib/objalloc.c
build/test-heap: tests/test_heap.c ../src/sysdolphin/baselib/aobj.c $(HEAP_SOURCES) ../src/sysdolphin/baselib/objalloc.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_heap.c ../src/sysdolphin/baselib/aobj.c $(HEAP_SOURCES) -o $@

ANIMATION_SOURCES = src/animation.c src/archive.c ../src/sysdolphin/baselib/fobj.c ../src/sysdolphin/baselib/spline.c
build/test-animation: tests/test_animation.c $(ANIMATION_SOURCES) $(HEAP_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_animation.c $(ANIMATION_SOURCES) $(HEAP_SOURCES) -o $@

build/inspect-disc: tools/inspect_disc.c src/disc.c include/melee_disc.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/inspect_disc.c src/disc.c -o $@

build/verify-disc: tools/verify_disc.c src/disc.c src/archive.c include/melee_disc.h include/melee_archive.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/verify_disc.c src/disc.c src/archive.c -o $@

build/test-disc: tests/test_disc.c src/disc.c include/melee_disc.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tests/test_disc.c src/disc.c -o $@

build/verify-motion: tools/verify_motion.c ../src/sysdolphin/baselib/aobj.c ../src/melee/lb/lbanim.c ../src/melee/lb/lbanim.h $(ANIMATION_SOURCES) $(HEAP_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -Wl,-dead_strip tools/verify_motion.c ../src/sysdolphin/baselib/aobj.c ../src/melee/lb/lbanim.c $(ANIMATION_SOURCES) $(HEAP_SOURCES) -o $@

build/test-hsd-mtx: tests/test_hsd_mtx.c ../src/sysdolphin/baselib/mtx.c ../src/sysdolphin/baselib/mtx.h src/mtx_backend.c $(MTX_SOURCES) $(HEAP_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_hsd_mtx.c ../src/sysdolphin/baselib/mtx.c src/mtx_backend.c $(MTX_SOURCES) $(HEAP_SOURCES) -o $@

DVD_SOURCES = src/dvd_backend.c src/disc.c src/os_interrupt.c src/archive.c
build/test-dvd: tests/test_dvd.c $(DVD_SOURCES) ../src/melee/lb/lbfile.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_dvd.c $(DVD_SOURCES) ../src/melee/lb/lbfile.c $(DIAGNOSTICS_SOURCES) -o $@

ARAM_SOURCES = src/aram_backend.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES) ../extern/dolphin/src/dolphin/ar/arq.c
build/test-aram: tests/test_aram.c $(ARAM_SOURCES) ../extern/dolphin/include/dolphin/ar.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined tests/test_aram.c $(ARAM_SOURCES) -o $@

DEVCOM_SOURCES = src/archive.c src/dvd_backend.c src/disc.c src/aram_backend.c src/os_interrupt.c src/cache_backend.c $(DIAGNOSTICS_SOURCES) src/heap_backend.c ../extern/dolphin/src/dolphin/ar/arq.c ../src/sysdolphin/baselib/devcom.c ../src/sysdolphin/baselib/memory.c
build/test-devcom: tests/test_devcom.c $(DEVCOM_SOURCES) ../src/sysdolphin/baselib/forward.h ../src/sysdolphin/baselib/devcom.static.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined tests/test_devcom.c $(DEVCOM_SOURCES) -o $@

build/test-texture: tests/test_texture.c src/texture.c include/melee_texture.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tests/test_texture.c src/texture.c -o $@

build/verify-textures: tools/verify_textures.c src/texture.c src/archive.c include/melee_texture.h include/melee_archive.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tools/verify_textures.c src/texture.c src/archive.c -o $@

CLASS_SOURCES = ../src/sysdolphin/baselib/class.c ../src/sysdolphin/baselib/object.c ../src/sysdolphin/baselib/hash.c
build/test-class: tests/test_class.c $(CLASS_SOURCES) $(HEAP_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_class.c $(CLASS_SOURCES) $(HEAP_SOURCES) -o $@

build/test-id: tests/test_id.c ../src/sysdolphin/baselib/id.c ../src/sysdolphin/baselib/id.h ../src/sysdolphin/baselib/forward.h $(HEAP_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined tests/test_id.c ../src/sysdolphin/baselib/id.c $(HEAP_SOURCES) -o $@

GOBJ_SOURCES = $(addprefix ../src/sysdolphin/baselib/,gobj.c gobjinit.c gobjproc.c gobjplink.c gobjgxlink.c gobjobject.c gobjuserdata.c)
build/test-gobj: tests/test_gobj.c $(GOBJ_SOURCES) $(wildcard ../src/sysdolphin/baselib/gobj*.h) $(HEAP_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_gobj.c $(GOBJ_SOURCES) $(HEAP_SOURCES) -o $@

JOINT_SOURCES = src/joint.c src/archive.c
build/test-joint: tests/test_joint.c $(JOINT_SOURCES) include/melee_joint.h include/melee_archive.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tests/test_joint.c $(JOINT_SOURCES) -o $@

build/verify-joints: tools/verify_joints.c $(JOINT_SOURCES) include/melee_joint.h include/melee_archive.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tools/verify_joints.c $(JOINT_SOURCES) -o $@

AOBJ_SOURCES = ../src/sysdolphin/baselib/aobj.c ../src/melee/lb/lbanim.c
build/test-aobj: tests/test_aobj.c $(AOBJ_SOURCES) $(ANIMATION_SOURCES) $(HEAP_SOURCES) ../src/melee/lb/lbanim.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_aobj.c $(AOBJ_SOURCES) $(ANIMATION_SOURCES) $(HEAP_SOURCES) -o $@

POSE_SOURCES = src/pose.c src/joint.c ../src/sysdolphin/baselib/jobj.c ../src/sysdolphin/baselib/robj.c ../src/sysdolphin/baselib/bytecode.c ../src/sysdolphin/baselib/util.c ../src/sysdolphin/baselib/list.c ../src/sysdolphin/baselib/random.c ../src/sysdolphin/baselib/mtx.c src/mtx_backend.c $(MTX_SOURCES) $(AOBJ_SOURCES) $(ANIMATION_SOURCES) $(HEAP_SOURCES)
build/test-pose: tests/test_pose.c $(POSE_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_pose.c $(POSE_SOURCES) -o $@

build/verify-poses: tools/verify_poses.c $(POSE_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tools/verify_poses.c $(POSE_SOURCES) -o $@

build/test-vertex: tests/test_vertex.c src/vertex.c include/melee_vertex.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tests/test_vertex.c src/vertex.c -o $@

build/verify-vertices: tools/verify_vertices.c src/vertex.c src/joint.c src/archive.c include/melee_vertex.h include/melee_joint.h include/melee_archive.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tools/verify_vertices.c src/vertex.c src/joint.c src/archive.c -o $@

MATERIAL_SOURCES = src/material.c src/texture.c ../src/sysdolphin/baselib/tobj.c
SKIN_SOURCES = src/skin.c src/vertex.c ../src/sysdolphin/baselib/displayfunc.c $(POSE_SOURCES)
build/test-skin: tests/test_skin.c $(SKIN_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_skin.c $(SKIN_SOURCES) -o $@

build/verify-skin: tools/verify_skin.c $(SKIN_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tools/verify_skin.c $(SKIN_SOURCES) -o $@

build/test-model: tests/test_model.c src/model.c $(MATERIAL_SOURCES) $(SKIN_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_model.c src/model.c $(MATERIAL_SOURCES) $(SKIN_SOURCES) -o $@

build/verify-model: tools/verify_model.c tools/render_bridge.c tools/RenderBridge.h src/model.c $(MATERIAL_SOURCES) $(SKIN_SOURCES) $(VISIBILITY_SOURCES) src/action.c src/fighter_command.c ../src/melee/lb/lbcommand.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tools/verify_model.c tools/render_bridge.c src/model.c $(MATERIAL_SOURCES) $(SKIN_SOURCES) $(VISIBILITY_SOURCES) src/action.c src/fighter_command.c ../src/melee/lb/lbcommand.c -o $@

build/test-material: tests/test_material.c $(MATERIAL_SOURCES) src/archive.c $(DIAGNOSTICS_SOURCES) ../src/sysdolphin/baselib/mtx.c src/mtx_backend.c $(MTX_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_material.c $(MATERIAL_SOURCES) src/archive.c $(DIAGNOSTICS_SOURCES) ../src/sysdolphin/baselib/mtx.c src/mtx_backend.c $(MTX_SOURCES) -o $@

test: build/test-material build/test-model build/test-skin build/test-vertex build/test-pose build/test-aobj build/test-joint build/test-gobj build/test-id build/test-class build/test-texture build/test-devcom build/test-aram build/test-dvd build/test-hsd-mtx build/test-disc build/test-animation build/test-heap build/test-native build/test-game-math build/test-hsd-input build/test-mtx build/test-game-abi build/test-effect-params build/test-animation-search build/test-particle-sort
	./build/test-vi-backend
	./build/test-hsd-vi
	./build/test-material
	./build/test-gx-pixel
	./build/test-visibility
	./build/test-command
	./build/test-action
	./build/test-model
	./build/test-skin
	./build/test-vertex
	./build/test-pose
	./build/test-aobj
	./build/test-joint
	./build/test-gobj
	./build/test-id
	./build/test-class
	./build/test-texture
	./build/test-devcom
	./build/test-aram
	./build/test-dvd
	./build/test-hsd-mtx
	./build/test-disc
	./build/test-animation
	./build/test-heap
	./build/test-native
	./build/test-game-math
	./build/test-hsd-input
	./build/test-mtx
	./build/test-game-abi
	./build/test-effect-params
	./build/test-animation-search
	./build/test-particle-sort

audit:
	python3 tools/audit.py

clean:
	rm -rf build

.PHONY: test-font-extract test-font-data
test: test-font-extract
test-font-extract:
	python3 tests/test_font_extract.py
ifneq ($(wildcard ../orig/GALE01/sys/main.dol),)
test: test-font-data
test-font-data:
	python3 tools/verify_fonts.py
endif

.PHONY: test-mutecity
test: test-mutecity
build/test-mutecity: tests/test_mutecity.c ../src/melee/gr/grmutecity.c ../src/melee/gr/grmutecity.static.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_mutecity.c ../src/melee/gr/grmutecity.c -o $@
test-mutecity: build/test-mutecity
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-mutecity

.PHONY: test-toy-layout
test: test-toy-layout
build/test-toy-layout: tests/test_toy_layout.c ../src/melee/ty/types.h ../src/melee/ty/forward.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O3 -fstrict-aliasing -g -fsanitize=address,undefined tests/test_toy_layout.c -o $@
test-toy-layout: build/test-toy-layout
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-toy-layout

.PHONY: test-jpeg-runtime
build/test-movie-stop: tests/test_movie_stop.c ../src/melee/lb/lbmthp.c src/vi_backend.c src/os_interrupt.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -pthread -fsanitize=address,undefined -Wl,-dead_strip tests/test_movie_stop.c src/vi_backend.c src/os_interrupt.c -o $@
.PHONY: test-movie-stop
test: test-movie-stop
test-movie-stop: build/test-movie-stop
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-movie-stop

test: test-jpeg-runtime
build/test-jpeg-runtime: tests/test_jpeg_runtime.c ../src/Runtime/Gecko_setjmp.h ../src/sysdolphin/baselib/hsd_3A94.h ../src/sysdolphin/baselib/hsd_3B33.c ../src/sysdolphin/baselib/hsd_4D11.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_jpeg_runtime.c ../src/sysdolphin/baselib/hsd_3B33.c ../src/sysdolphin/baselib/hsd_4D11.c -o $@
test-jpeg-runtime: build/test-jpeg-runtime
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-jpeg-runtime

.PHONY: test-diagnostics
test: test-diagnostics
DIAGNOSTICS_TEST_SOURCES = tests/test_diagnostics.c ../src/melee/db/dberror.c ../src/sysdolphin/baselib/hsd_393C.c
build/test-diagnostics: $(DIAGNOSTICS_TEST_SOURCES) $(DIAGNOSTICS_SOURCES) ../src/sysdolphin/baselib/debug.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip $(DIAGNOSTICS_TEST_SOURCES) $(DIAGNOSTICS_SOURCES) -o $@
test-diagnostics: build/test-diagnostics
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-diagnostics

.PHONY: test-arena
test: test-arena
build/test-arena: tests/test_arena.c ../extern/dolphin/src/dolphin/os/OSArena.c $(HEAP_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_arena.c ../extern/dolphin/src/dolphin/os/OSArena.c $(HEAP_SOURCES) -o $@
test-arena: build/test-arena
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-arena

.PHONY: test-time
test: test-time
TIME_SOURCES = src/time_backend.c ../extern/dolphin/src/dolphin/os/OSTime.c ../src/melee/lb/lbtime.c
build/test-time: tests/test_time.c $(TIME_SOURCES) $(DIAGNOSTICS_SOURCES) ../extern/dolphin/include/dolphin/os.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -I../extern/dolphin/src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_time.c $(TIME_SOURCES) $(DIAGNOSTICS_SOURCES) -o $@
test-time: build/test-time
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-time

.PHONY: test-runtime
test: test-runtime
build/test-runtime: tests/test_runtime.c src/runtime_backend.c ../src/MSL/float.c ../src/melee/gm/gm_1832.c ../src/Runtime/runtime.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_runtime.c src/runtime_backend.c ../src/MSL/float.c ../src/melee/gm/gm_1832.c -o $@
test-runtime: build/test-runtime
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-runtime

.PHONY: test-alarm
test: test-alarm
ALARM_SOURCES = src/alarm_backend.c src/time_backend.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES)
build/test-alarm: tests/test_alarm.c $(ALARM_SOURCES) $(NATIVE_HEADERS) ../extern/dolphin/include/dolphin/os/OSAlarm.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_alarm.c $(ALARM_SOURCES) -o $@
test-alarm: build/test-alarm
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-alarm

.PHONY: test-input-timer
test: test-input-timer
INPUT_TIMER_SOURCES = tests/test_input_timer.c ../src/melee/lb/lb_0195.c ../src/melee/lb/lbcardgame.c ../src/melee/lb/lbsnap.c src/pad_backend.c src/hsd_input.c ../src/sysdolphin/baselib/controller.c ../src/sysdolphin/baselib/rumble.c $(ALARM_SOURCES)
build/test-input-timer: $(INPUT_TIMER_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip $(INPUT_TIMER_SOURCES) -o $@
test-input-timer: build/test-input-timer
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-input-timer

.PHONY: test-boot
test: test-boot
BOOT_SOURCES = tests/test_boot.c src/os_boot_backend.c src/time_backend.c ../extern/dolphin/src/dolphin/os/OSArena.c ../src/sysdolphin/baselib/synth.c $(HEAP_SOURCES)
build/test-boot: $(BOOT_SOURCES) ../src/sysdolphin/baselib/initialize.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip $(BOOT_SOURCES) -o $@
test-boot: build/test-boot
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-boot

.PHONY: test-video
test: test-video
build/test-video: tests/test_video.c ../src/sysdolphin/baselib/video.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_video.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES) -o $@
test-video: build/test-video
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-video

.PHONY: test-settings
test: test-settings
build/test-settings: tests/test_settings.c src/settings_backend.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_settings.c src/settings_backend.c $(DIAGNOSTICS_SOURCES) -o $@
test-settings: build/test-settings
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-settings

.PHONY: test-axfx-delay
test: test-axfx-delay
AXFX_DELAY_SOURCES = ../extern/dolphin/src/dolphin/axfx/delay.c ../extern/dolphin/src/dolphin/axfx/axfx.c src/os_interrupt.c src/heap_backend.c $(DIAGNOSTICS_SOURCES)
build/test-axfx-delay: tests/test_axfx_delay.c $(AXFX_DELAY_SOURCES) ../extern/dolphin/include/dolphin/axfx.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_axfx_delay.c $(AXFX_DELAY_SOURCES) -o $@
test-axfx-delay: build/test-axfx-delay
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-axfx-delay

.PHONY: test-reverb-std
test: test-reverb-std
build/reverb_reference.inc: tools/generate_reverb_reference.py ../extern/dolphin/src/dolphin/axfx/reverb_std.c | build
	python3 tools/generate_reverb_reference.py $@
REVERB_STD_SOURCES = src/reverb_std_backend.c ../extern/dolphin/src/dolphin/axfx/axfx.c src/os_interrupt.c src/heap_backend.c $(DIAGNOSTICS_SOURCES)
build/test-reverb-std: tests/test_reverb_std.c $(REVERB_STD_SOURCES) build/reverb_reference.inc ../extern/dolphin/include/dolphin/axfx.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -Ibuild -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_reverb_std.c $(REVERB_STD_SOURCES) -o $@
test-reverb-std: build/test-reverb-std
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-reverb-std

.PHONY: test-reverb-hi
test: test-reverb-hi
build/reverb_hi_reference.inc: tools/generate_reverb_reference.py ../extern/dolphin/src/dolphin/axfx/reverb_hi.c | build
	python3 tools/generate_reverb_reference.py $@ hi
build/reverb_cross_reference.inc: tools/generate_reverb_reference.py ../extern/dolphin/src/dolphin/axfx/reverb_hi.c | build
	python3 tools/generate_reverb_reference.py $@ cross
REVERB_HI_SOURCES = src/reverb_hi_backend.c ../extern/dolphin/src/dolphin/axfx/axfx.c src/os_interrupt.c src/heap_backend.c $(DIAGNOSTICS_SOURCES)
build/test-reverb-hi: tests/test_reverb_hi.c $(REVERB_HI_SOURCES) build/reverb_hi_reference.inc build/reverb_cross_reference.inc ../extern/dolphin/include/dolphin/axfx.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -Ibuild -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_reverb_hi.c $(REVERB_HI_SOURCES) -o $@
test-reverb-hi: build/test-reverb-hi
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-reverb-hi

.PHONY: test-axdriver-aux
test: test-axdriver-aux
AXDRIVER_AUX_SOURCES = ../extern/dolphin/src/dolphin/axfx/chorus.c $(AXFX_DELAY_SOURCES) src/reverb_std_backend.c src/reverb_hi_backend.c
build/test-axdriver-aux: tests/test_axdriver_aux.c ../src/sysdolphin/baselib/axdriver.c ../src/sysdolphin/baselib/axdriver.static.h $(AXDRIVER_AUX_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_axdriver_aux.c $(AXDRIVER_AUX_SOURCES) -o $@
test-axdriver-aux: build/test-axdriver-aux
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-axdriver-aux

.PHONY: test-chorus
test: test-chorus
build/chorus_src1_reference.inc: tools/generate_reverb_reference.py ../extern/dolphin/src/dolphin/axfx/chorus.c | build
	python3 tools/generate_reverb_reference.py $@ src1
build/chorus_src2_reference.inc: tools/generate_reverb_reference.py ../extern/dolphin/src/dolphin/axfx/chorus.c | build
	python3 tools/generate_reverb_reference.py $@ src2
CHORUS_TEST_SOURCES = ../extern/dolphin/src/dolphin/axfx/axfx.c src/os_interrupt.c src/heap_backend.c $(DIAGNOSTICS_SOURCES)
build/test-chorus: tests/test_chorus.c ../extern/dolphin/src/dolphin/axfx/chorus.c $(CHORUS_TEST_SOURCES) build/chorus_src1_reference.inc build/chorus_src2_reference.inc $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -Ibuild -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_chorus.c $(CHORUS_TEST_SOURCES) -o $@
test-chorus: build/test-chorus
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-chorus

.PHONY: test-ax-voices
test: test-ax-voices
AX_VOICE_SOURCES = src/ax_voice_backend.c ../extern/dolphin/src/dolphin/ax/AXAlloc.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES)
build/test-ax-voices: tests/test_ax_voices.c $(AX_VOICE_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_ax_voices.c $(AX_VOICE_SOURCES) -o $@
test-ax-voices: build/test-ax-voices
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ax-voices

.PHONY: test-ax-parameters
test: test-ax-parameters
build/test-ax-parameters: tests/test_ax_parameters.c ../extern/dolphin/src/dolphin/ax/AXVPB.c $(AX_VOICE_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_ax_parameters.c ../extern/dolphin/src/dolphin/ax/AXVPB.c $(AX_VOICE_SOURCES) -o $@
test-ax-parameters: build/test-ax-parameters
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ax-parameters

.PHONY: test-ax-decode
test: test-ax-decode
AX_DECODE_SOURCES = src/ax_decode.c src/aram_backend.c ../extern/dolphin/src/dolphin/ax/AXVPB.c $(AX_VOICE_SOURCES)
build/test-ax-decode: tests/test_ax_decode.c $(AX_DECODE_SOURCES) src/ax_coefficients.inc $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_ax_decode.c $(AX_DECODE_SOURCES) -o $@
test-ax-decode: build/test-ax-decode
	python3 tools/prepare_ax_coefficients.py --check
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ax-decode

.PHONY: test-ax-mix
test: test-ax-mix
build/test-ax-mix: tests/test_ax_mix.c src/ax_mix.c src/ax_itd.c $(AX_DECODE_SOURCES) src/ax_coefficients.inc $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_ax_mix.c src/ax_mix.c src/ax_itd.c $(AX_DECODE_SOURCES) -o $@
test-ax-mix: build/test-ax-mix
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ax-mix

.PHONY: test-ax-aux
test: test-ax-aux
AX_AUX_SOURCES = src/ax_aux_backend.c ../extern/dolphin/src/dolphin/ax/AXAux.c src/cache_backend.c ../extern/dolphin/src/dolphin/ax/AXVPB.c src/ax_voice_backend.c ../extern/dolphin/src/dolphin/ax/AXAlloc.c $(AXFX_DELAY_SOURCES)
build/test-ax-aux: tests/test_ax_aux.c $(AX_AUX_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_ax_aux.c $(AX_AUX_SOURCES) -o $@
test-ax-aux: build/test-ax-aux
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ax-aux

.PHONY: test-ax-depop
test: test-ax-depop
build/test-ax-depop: tests/test_ax_depop.c src/ax_depop.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_ax_depop.c src/ax_depop.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES) -o $@
test-ax-depop: build/test-ax-depop
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ax-depop

.PHONY: test-ax-pool-mix
test: test-ax-pool-mix
AX_POOL_MIX_SOURCES = src/audio_producer.c src/audio_ring.c src/ax_stream.c src/ax_output.c src/ax_aux_backend.c ../extern/dolphin/src/dolphin/ax/AXAux.c src/cache_backend.c src/ax_pool_mix.c src/ax_mix.c src/ax_itd.c src/ax_depop.c $(AX_DECODE_SOURCES)
build/test-ax-pool-mix: tests/test_ax_pool_mix.c $(AX_POOL_MIX_SOURCES) src/ax_coefficients.inc $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_ax_pool_mix.c $(AX_POOL_MIX_SOURCES) -o $@
test-ax-pool-mix: build/test-ax-pool-mix
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ax-pool-mix

build/test-ax-pool-mix-tsan: tests/test_ax_pool_mix.c $(AX_POOL_MIX_SOURCES) src/ax_coefficients.inc $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=thread tests/test_ax_pool_mix.c $(AX_POOL_MIX_SOURCES) -o $@

.PHONY: test-audio-ring
test: test-audio-ring
build/test-audio-ring: tests/test_audio_ring.c src/audio_ring.c include/melee_audio_ring.h | build
	$(CC) $(CPPFLAGS) -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_audio_ring.c -o $@
build/test-audio-ring-tsan: tests/test_audio_ring.c src/audio_ring.c include/melee_audio_ring.h | build
	$(CC) $(CPPFLAGS) -std=gnu11 -O2 -g -fsanitize=thread tests/test_audio_ring.c -o $@
test-audio-ring: build/test-audio-ring build/test-audio-ring-tsan
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-audio-ring
	TSAN_OPTIONS=halt_on_error=1 ./build/test-audio-ring-tsan

.PHONY: test-sem
.PHONY: test-ssm
test: test-ssm
build/test-ssm: tests/test_ssm.c src/ssm.c src/disc.c $(AX_DECODE_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_ssm.c src/ssm.c src/disc.c $(AX_DECODE_SOURCES) -o $@
test-ssm: build/test-ssm
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ssm

test: test-sem
build/test-sem: tests/test_sem.c src/sem.c ../src/sysdolphin/baselib/axdriver.c ../src/sysdolphin/baselib/axdriver.static.h $(DVD_SOURCES) $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_sem.c src/sem.c $(DVD_SOURCES) $(DIAGNOSTICS_SOURCES) -o $@
test-sem: build/test-sem
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-sem

.PHONY: test-synth-bank-range
.PHONY: test-synth-start
.PHONY: test-synth-compact
SYNTH_COMPACT_SOURCES = $(sort $(AX_POOL_MIX_SOURCES) $(DEVCOM_SOURCES))
.PHONY: test-synth-load
.PHONY: test-ax-itd
test: test-ax-itd
build/test-ax-itd: tests/test_ax_itd.c src/ax_itd.c include/melee_ax_itd.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_ax_itd.c src/ax_itd.c -o $@
test-ax-itd: build/test-ax-itd
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-ax-itd
SYNTH_LOAD_SOURCES = src/ssm.c src/ai_backend.c src/settings_backend.c $(SYNTH_COMPACT_SOURCES)
SOUND_DRIVER_SOURCES = $(sort $(SYNTH_LOAD_SOURCES) $(AXDRIVER_AUX_SOURCES) src/sem.c ../src/sysdolphin/baselib/synth.c)
build/test-sound-driver: tests/test_sound_driver.c ../src/sysdolphin/baselib/axdriver.c ../src/sysdolphin/baselib/axdriver.static.h $(SOUND_DRIVER_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_sound_driver.c $(SOUND_DRIVER_SOURCES) -o $@
build/test-sound-driver-tsan: tests/test_sound_driver.c ../src/sysdolphin/baselib/axdriver.c ../src/sysdolphin/baselib/axdriver.static.h $(SOUND_DRIVER_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=thread -Wl,-dead_strip tests/test_sound_driver.c $(SOUND_DRIVER_SOURCES) -o $@
test: test-synth-load
build/test-synth-load: tests/test_synth_load.c ../src/sysdolphin/baselib/synth.c ../src/sysdolphin/baselib/synth.static.h $(SYNTH_LOAD_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_synth_load.c $(SYNTH_LOAD_SOURCES) -o $@
test-synth-load: build/test-synth-load
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-synth-load
build/test-synth-load-tsan: tests/test_synth_load.c ../src/sysdolphin/baselib/synth.c ../src/sysdolphin/baselib/synth.static.h $(SYNTH_LOAD_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=thread -Wl,-dead_strip tests/test_synth_load.c $(SYNTH_LOAD_SOURCES) -o $@

test: test-synth-compact
build/test-synth-compact: tests/test_synth_compact.c ../src/sysdolphin/baselib/synth.c ../src/sysdolphin/baselib/synth.static.h $(SYNTH_COMPACT_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_synth_compact.c $(SYNTH_COMPACT_SOURCES) -o $@
test-synth-compact: build/test-synth-compact
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-synth-compact

test: test-synth-start
build/test-synth-start: tests/test_synth_start.c src/ssm.c src/disc.c ../src/sysdolphin/baselib/synth.c ../src/sysdolphin/baselib/synth.static.h $(AX_POOL_MIX_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_synth_start.c src/ssm.c src/disc.c $(AX_POOL_MIX_SOURCES) -o $@
test-synth-start: build/test-synth-start
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-synth-start

test: test-synth-bank-range
build/test-synth-bank-range: tests/test_synth_bank_range.c ../src/sysdolphin/baselib/synth.c ../src/sysdolphin/baselib/synth.static.h src/os_interrupt.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_synth_bank_range.c src/os_interrupt.c -o $@
test-synth-bank-range: build/test-synth-bank-range
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-synth-bank-range

# Run separately where Apple's audio components are available; offline only.
build/test-audio-output: apple/AudioOutput.swift apple/AudioBridge.h tests/test_audio_output.swift tools/build_audio_output_test.py $(AX_POOL_MIX_SOURCES) $(NATIVE_HEADERS) | build
	python3 tools/build_audio_output_test.py $(AX_POOL_MIX_SOURCES)

VI_BACKEND_SOURCES = src/vi_backend.c src/os_interrupt.c
build/test-vi-backend: tests/test_vi_backend.c $(VI_BACKEND_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tests/test_vi_backend.c $(VI_BACKEND_SOURCES) -o $@
build/test-vi-backend-tsan: tests/test_vi_backend.c $(VI_BACKEND_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=thread tests/test_vi_backend.c $(VI_BACKEND_SOURCES) -o $@
test: build/test-vi-backend

build/test-hsd-vi: tests/test_hsd_vi.c ../src/sysdolphin/baselib/video.c $(VI_BACKEND_SOURCES) $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_hsd_vi.c $(VI_BACKEND_SOURCES) $(DIAGNOSTICS_SOURCES) -o $@
build/test-hsd-vi-tsan: tests/test_hsd_vi.c ../src/sysdolphin/baselib/video.c $(VI_BACKEND_SOURCES) $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=thread -Wl,-dead_strip tests/test_hsd_vi.c $(VI_BACKEND_SOURCES) $(DIAGNOSTICS_SOURCES) -o $@
test: build/test-hsd-vi

build/test-card-store: tests/test_card_store.c src/card_store.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -O2 -fsanitize=address,undefined tests/test_card_store.c src/card_store.c -o $@
build/test-card-store-tsan: tests/test_card_store.c src/card_store.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -O2 -fsanitize=thread tests/test_card_store.c src/card_store.c -o $@
.PHONY: test-card-store
test: test-card-store
test-card-store: build/test-card-store
	./build/test-card-store

CARD_BACKEND_SOURCES = src/card_backend.c src/card_store.c src/os_interrupt.c
build/test-card-backend: tests/test_card_backend.c $(CARD_BACKEND_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -O2 -fsanitize=address,undefined tests/test_card_backend.c $(CARD_BACKEND_SOURCES) -o $@
build/test-card-backend-tsan: tests/test_card_backend.c $(CARD_BACKEND_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -O2 -fsanitize=thread tests/test_card_backend.c $(CARD_BACKEND_SOURCES) -o $@
.PHONY: test-card-backend
test: test-card-backend
test-card-backend: build/test-card-backend
	./build/test-card-backend

DEVKIT_SOURCES = src/devkit_backend.c ../src/sysdolphin/baselib/hsd_392C.c ../src/sysdolphin/baselib/hsd_3933.c $(DIAGNOSTICS_SOURCES)
build/test-devkit-backend: tests/test_devkit_backend.c $(DEVKIT_SOURCES) $(NATIVE_HEADERS) ../extern/dolphin/include/dolphin/mcc.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_devkit_backend.c $(DEVKIT_SOURCES) -o $@
.PHONY: test-devkit-backend
test: test-devkit-backend
test-devkit-backend: build/test-devkit-backend
	./build/test-devkit-backend

build/test-thread-diagnostics: tests/test_thread_diagnostics.c ../src/melee/db/dbinit.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_thread_diagnostics.c ../src/melee/db/dbinit.c -o $@
.PHONY: test-thread-diagnostics
test: test-thread-diagnostics
test-thread-diagnostics: build/test-thread-diagnostics
	./build/test-thread-diagnostics

build/test-thp-decode: tests/test_thp_decode.cpp src/thp_decode.cpp include/melee_thp.h | build
	xcrun clang++ -std=c++20 -Iinclude -Wall -Wextra -Werror -O2 -g -fsanitize=address,undefined tests/test_thp_decode.cpp src/thp_decode.cpp -o $@
.PHONY: test-thp-decode
test: test-thp-decode
test-thp-decode: build/test-thp-decode
	./build/test-thp-decode

# Independent pixel oracle for local retail verification; not an app dependency.
JPEG_PREFIX ?= /opt/homebrew/opt/jpeg-turbo
build/verify-thp-pixels: tools/verify_thp_pixels.cpp src/thp_decode.cpp include/melee_thp.h | build
	xcrun clang++ -std=c++20 -Iinclude -I$(JPEG_PREFIX)/include -L$(JPEG_PREFIX)/lib -ljpeg -O2 -g -fsanitize=address,undefined tools/verify_thp_pixels.cpp src/thp_decode.cpp -o $@

build/game-thp-still.o: tests/game_thp_still.c ../src/melee/lb/lb_01F8.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -c $< -o $@
build/test-game-thp-still: build/game-thp-still.o tests/test_thp_decode.cpp src/thp_decode.cpp include/melee_thp.h
	xcrun clang++ -std=c++20 -DMELEE_TEST_STILL -Iinclude -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_thp_decode.cpp src/thp_decode.cpp build/game-thp-still.o -o $@
test: test-game-thp-still
.PHONY: test-game-thp-still
test-game-thp-still: build/test-game-thp-still
	./build/test-game-thp-still

build/test-mth-header: tests/test_mth_header.c src/mth_header.c src/disc.c ../src/melee/lb/lbmthp.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_mth_header.c src/mth_header.c src/disc.c -o $@
test: test-mth-header
.PHONY: test-reset-backend
test: test-reset-backend
build/test-reset-backend: tests/test_reset_backend.c src/reset_backend.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_reset_backend.c src/reset_backend.c src/os_interrupt.c $(DIAGNOSTICS_SOURCES) -o $@
test-reset-backend: build/test-reset-backend
	./build/test-reset-backend

build/test-lbmemory-handles: tests/test_lbmemory_handles.c ../src/melee/lb/lbmemory.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip $< -o $@
test: test-lbmemory-handles
.PHONY: test-lbmemory-handles
test-lbmemory-handles: build/test-lbmemory-handles
	./build/test-lbmemory-handles

build/test-rumble-bank: tests/test_rumble_bank.c src/rumble_bank.c src/archive.c ../src/sysdolphin/baselib/rumble.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_rumble_bank.c src/rumble_bank.c src/archive.c ../src/sysdolphin/baselib/rumble.c -o $@
test: test-rumble-bank
.PHONY: test-rumble-bank
test-rumble-bank: build/test-rumble-bank
	./build/test-rumble-bank

build/test-card-icons: tests/test_card_icons.c src/card_icons.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_card_icons.c src/card_icons.c src/archive.c -o $@
test: test-card-icons
.PHONY: test-card-icons
test-card-icons: build/test-card-icons
	./build/test-card-icons
.PHONY: test-mth-header
test-mth-header: build/test-mth-header
	./build/test-mth-header

build/mth-stream-test.o: tests/test_mth_stream.c ../src/melee/lb/lbmthp.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -c $< -o $@
build/test-mth-stream: build/mth-stream-test.o src/mth_header.c src/os_interrupt.c src/thp_decode.cpp include/melee_thp.h
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -c src/mth_header.c -o build/mth-stream-header.o
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -c src/os_interrupt.c -o build/mth-stream-interrupt.o
	xcrun clang++ -std=c++20 -Iinclude -O2 -g -fsanitize=address,undefined -Wl,-dead_strip src/thp_decode.cpp build/mth-stream-test.o build/mth-stream-header.o build/mth-stream-interrupt.o -o $@

build/test-camera: tests/test_camera.c src/camera.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_camera.c src/camera.c src/archive.c -o $@
.PHONY: test-camera
test: test-camera
test-camera: build/test-camera
	./build/test-camera

build/test-lights: tests/test_lights.c src/lights.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_lights.c src/lights.c src/archive.c -o $@
.PHONY: test-lights
test: test-lights
test-lights: build/test-lights
	./build/test-lights

build/test-card-requests: tests/test_card_requests.c ../src/sysdolphin/baselib/hsd_3A94.c ../src/sysdolphin/baselib/hsd_3B27.c ../src/sysdolphin/baselib/hsd_4D11.c src/os_interrupt.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_card_requests.c ../src/sysdolphin/baselib/hsd_3A94.c ../src/sysdolphin/baselib/hsd_3B27.c ../src/sysdolphin/baselib/hsd_4D11.c src/os_interrupt.c -o $@
.PHONY: test-card-requests
test: test-card-requests
test-card-requests: build/test-card-requests
	./build/test-card-requests

build/test-sis-bank: tests/test_sis_bank.c src/sis_bank.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_sis_bank.c src/sis_bank.c src/archive.c -o $@
.PHONY: test-sis-bank
test: test-sis-bank
test-sis-bank: build/test-sis-bank
	./build/test-sis-bank

build/test-trophy-data: tests/test_trophy_data.c src/trophy_data.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_trophy_data.c src/trophy_data.c src/archive.c -o $@
.PHONY: test-trophy-data
test: test-trophy-data
test-trophy-data: build/test-trophy-data
	./build/test-trophy-data

build/test-sis-measure: tests/test_sis_measure.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_sis_measure.c -o $@
.PHONY: test-sis-measure
test: test-sis-measure
test-sis-measure: build/test-sis-measure
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-sis-measure

.PHONY: test-hps-start
test: test-hps-start
build/test-hps-start: tests/test_hps_start.c ../src/sysdolphin/baselib/synth.c ../src/sysdolphin/baselib/synth.static.h $(AX_POOL_MIX_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_hps_start.c $(AX_POOL_MIX_SOURCES) -o $@
test-hps-start: build/test-hps-start
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-hps-start

.PHONY: test-event-data
test: test-event-data
build/test-event-data: tests/test_event_data.c src/event_data.c src/archive.c ../src/melee/gm/gmevent_data.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined tests/test_event_data.c src/event_data.c src/archive.c -o $@
test-event-data: build/test-event-data
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-event-data

.PHONY: test-audio-load-data
test: test-audio-load-data
build/test-audio-load-data: tests/test_audio_load_data.c src/audio_load_data.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -g -fsanitize=address,undefined tests/test_audio_load_data.c src/audio_load_data.c src/archive.c -o $@
test-audio-load-data: build/test-audio-load-data
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-audio-load-data

.PHONY: test-sis-allocator
test: test-sis-allocator
build/test-sis-allocator: tests/test_sis_allocator.c ../src/sysdolphin/baselib/sislib.c ../src/sysdolphin/baselib/hsd_3A64.c src/sis_bank.c src/archive.c $(DIAGNOSTICS_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_sis_allocator.c ../src/sysdolphin/baselib/sislib.c ../src/sysdolphin/baselib/hsd_3A64.c src/sis_bank.c src/archive.c $(DIAGNOSTICS_SOURCES) -o $@
test-sis-allocator: build/test-sis-allocator
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-sis-allocator

.PHONY: test-refraction
test: test-refraction
build/test-refraction: tests/test_refraction.c src/refraction.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined tests/test_refraction.c src/refraction.c src/archive.c -o $@
test-refraction: build/test-refraction
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-refraction

.PHONY: test-particle-bank
test: test-particle-bank
build/test-particle-bank: tests/test_particle_bank.c src/particle_bank.c src/archive.c src/texture.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_particle_bank.c src/particle_bank.c src/archive.c src/texture.c -o $@
test-particle-bank: build/test-particle-bank
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-particle-bank

.PHONY: test-player-params
test: test-player-params
build/test-player-params: tests/test_player_params.c src/player_params.c src/archive.c ../src/melee/pl/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_player_params.c src/player_params.c src/archive.c -o $@
test-player-params: build/test-player-params
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-player-params

.PHONY: test-stage-collision
test: test-stage-collision
build/test-stage-collision: tests/test_stage_collision.c src/stage_collision.c src/archive.c ../src/melee/mp/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_stage_collision.c src/stage_collision.c src/archive.c -o $@
test-stage-collision: build/test-stage-collision
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-stage-collision

.PHONY: test-stage-params
test: test-stage-params
build/test-stage-params: tests/test_stage_params.c src/stage_params.c src/archive.c ../src/melee/gr/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_stage_params.c src/stage_params.c src/archive.c -o $@
test-stage-params: build/test-stage-params
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-stage-params

.PHONY: test-onett-params
test: test-onett-params
build/test-onett-params: tests/test_onett_params.c src/onett_params.c src/archive.c ../src/melee/gr/gronett.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_onett_params.c src/onett_params.c src/archive.c -o $@
test-onett-params: build/test-onett-params
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-onett-params

.PHONY: test-item-scripts
test: test-item-scripts
build/test-item-scripts: tests/test_item_scripts.c src/item_scripts.c src/archive.c ../src/melee/lb/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_item_scripts.c src/item_scripts.c src/archive.c -o $@
test-item-scripts: build/test-item-scripts
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-item-scripts

.PHONY: test-item-common
.PHONY: test-fighter-common
build/test-fighter-command: tests/test_fighter_command.c src/fighter_command.c ../src/melee/ft/ftaction.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_fighter_command.c src/fighter_command.c -o $@
build/test-character-motions: tests/test_character_motions.c ../src/melee/ft/ftdata.c src/character_motions.c src/animation.c src/archive.c $(ACTION_SOURCES) $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_character_motions.c ../src/melee/ft/ftdata.c src/character_motions.c src/animation.c src/archive.c $(ACTION_SOURCES) -o $@
build/test-character-wait: tests/test_character_wait.c src/character_wait.c src/archive.c ../src/melee/ft/ftwaitanim.c $(NATIVE_HEADERS) ../src/melee/ft/ftwaitanim.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_character_wait.c src/character_wait.c src/archive.c ../src/melee/ft/ftwaitanim.c -o $@
build/test-character-sounds: tests/test_character_sounds.c src/character_sounds.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_character_sounds.c src/character_sounds.c src/archive.c -o $@
build/test-character-aux: tests/test_character_aux.c src/character_aux.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_character_aux.c src/character_aux.c src/archive.c -o $@
build/test-character-collision: tests/test_character_collision.c src/character_collision.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_character_collision.c src/character_collision.c src/archive.c -o $@
build/test-character-attributes: tests/test_character_attributes.c src/character_attributes.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_character_attributes.c src/character_attributes.c src/archive.c -o $@
build/test-fighter-cpu: tests/test_fighter_cpu.c src/fighter_cpu.c src/archive.c ../src/melee/ft/ftcmdscript.c ../src/melee/ft/ftcpuattack.c $(NATIVE_HEADERS) ../src/melee/ft/ftcpuattack.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_fighter_cpu.c src/fighter_cpu.c src/archive.c ../src/melee/ft/ftcmdscript.c ../src/melee/ft/ftcpuattack.c -o $@
build/test-fighter-aux: tests/test_fighter_aux.c src/fighter_aux.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_fighter_aux.c src/fighter_aux.c src/archive.c -o $@
build/test-fighter-modifiers: tests/test_fighter_modifiers.c src/fighter_modifiers.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_fighter_modifiers.c src/fighter_modifiers.c src/archive.c -o $@
build/test-fighter-parts: tests/test_fighter_parts.c src/fighter_parts.c src/archive.c ../src/melee/ft/ftparts.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_fighter_parts.c src/fighter_parts.c src/archive.c ../src/melee/ft/ftparts.c -o $@
build/test-fighter-shakes: tests/test_fighter_shakes.c src/fighter_shakes.c src/archive.c ../src/melee/ft/kinds/ftCommon/ftCo_DamageFall.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_fighter_shakes.c src/fighter_shakes.c src/archive.c ../src/melee/ft/kinds/ftCommon/ftCo_DamageFall.c -o $@
test: test-fighter-common
build/test-fighter-common: tests/test_fighter_common.c src/fighter_common.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_fighter_common.c src/fighter_common.c src/archive.c -o $@
test-fighter-common: build/test-fighter-common
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-fighter-common

test: test-item-common
build/test-item-common: tests/test_item_common.c src/item_common.c src/archive.c ../src/melee/it/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_item_common.c src/item_common.c src/archive.c -o $@
test-item-common: build/test-item-common
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-item-common

.PHONY: test-item-attributes
test: test-item-attributes
build/test-item-attributes: tests/test_item_attributes.c src/item_attributes.c src/archive.c ../src/melee/it/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_item_attributes.c src/item_attributes.c src/archive.c -o $@
test-item-attributes: build/test-item-attributes
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-item-attributes

.PHONY: test-item-hurtbones
test: test-item-hurtbones
build/test-item-hurtbones: tests/test_item_hurtbones.c src/item_hurtbones.c src/archive.c ../src/melee/it/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_item_hurtbones.c src/item_hurtbones.c src/archive.c -o $@
test-item-hurtbones: build/test-item-hurtbones
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-item-hurtbones

.PHONY: test-item-dynamics
test: test-item-dynamics
build/test-item-dynamics: tests/test_item_dynamics.c src/item_dynamics.c src/archive.c ../src/melee/lb/lb_00F9.c ../src/melee/it/types.h ../src/melee/lb/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_item_dynamics.c src/item_dynamics.c src/archive.c ../src/melee/lb/lb_00F9.c -o $@
test-item-dynamics: build/test-item-dynamics
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-item-dynamics

build/test-item-script-graph: tests/test_item_script_graph.c src/item_scripts.c src/archive.c ../src/melee/lb/lbcommand.c ../src/melee/it/itanimlist.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_item_script_graph.c src/item_scripts.c src/archive.c ../src/melee/lb/lbcommand.c ../src/melee/it/itanimlist.c -o $@

build/test-foods-runtime: tests/test_foods_runtime.c ../src/melee/it/kinds/itfoods.c ../src/melee/it/itCommonItems.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_foods_runtime.c ../src/melee/it/kinds/itfoods.c -o $@

build/test-item-special: tests/test_item_special.c src/item_special.c src/archive.c $(NATIVE_HEADERS) ../src/melee/it/itCommonItems.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_item_special.c src/item_special.c src/archive.c -o $@

build/test-item-colors: tests/test_item_colors.c src/item_colors.c src/archive.c ../src/melee/lb/lb_013B.c ../src/melee/lb/lbcommand.c $(NATIVE_HEADERS) ../src/melee/lb/types.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_item_colors.c src/item_colors.c src/archive.c ../src/melee/lb/lb_013B.c ../src/melee/lb/lbcommand.c -o $@
build/test-fighter-colors: tests/test_fighter_colors.c src/item_colors.c src/archive.c ../src/melee/lb/lb_013B.c ../src/melee/lb/lbcommand.c $(NATIVE_HEADERS) ../src/melee/lb/types.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_fighter_colors.c src/item_colors.c src/archive.c ../src/melee/lb/lb_013B.c ../src/melee/lb/lbcommand.c -o $@

build/test-fighter-graph: tests/test_fighter_graph.c $(ACTION_SOURCES) src/archive.c $(NATIVE_HEADERS) ../src/melee/lb/types.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_fighter_graph.c $(ACTION_SOURCES) src/archive.c -o $@

build/test-character-dynamics: tests/test_character_dynamics.c src/character_dynamics.c src/item_dynamics.c src/animation.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_character_dynamics.c src/character_dynamics.c src/item_dynamics.c src/animation.c src/archive.c -o $@

build/test-character-parts: tests/test_character_parts.c ../src/melee/ft/ftparts.c ../src/sysdolphin/baselib/dobj.c src/character_parts.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_character_parts.c ../src/melee/ft/ftparts.c ../src/sysdolphin/baselib/dobj.c src/character_parts.c src/archive.c -o $@

build/test-character-part-anims: tests/test_character_part_anims.c src/character_part_anims.c src/animation.c src/archive.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_character_part_anims.c src/character_part_anims.c src/animation.c src/archive.c -o $@

build/test-collision-indices: tests/test_collision_indices.c ../src/melee/mp/mplib.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_collision_indices.c -o $@

build/test-bg-colors: ../src/melee/lb/lb_0219.c tests/test_bg_colors.c src/item_colors.c src/archive.c ../src/melee/lb/lb_013B.c ../src/melee/lb/lbcommand.c $(NATIVE_HEADERS) ../src/melee/lb/types.h | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_bg_colors.c src/item_colors.c src/archive.c ../src/melee/lb/lb_013B.c ../src/melee/lb/lbcommand.c -o $@

build/test-hud-frames: tests/test_hud_frames.c ../src/melee/gm/gm_1601.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_hud_frames.c -o $@

build/test-flag-byte: tests/test_flag_byte.c ../src/melee/gm/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_flag_byte.c -o $@

build/test-particle-cleanup: tests/test_particle_cleanup.c ../src/sysdolphin/baselib/particle.c $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined -Wl,-dead_strip tests/test_particle_cleanup.c -o $@

.PHONY: test-match-runtime
test: test-match-runtime
test-match-runtime: build/test-hud-frames build/test-flag-byte build/test-particle-cleanup build/test-sis-allocator
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-hud-frames
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-flag-byte
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-particle-cleanup
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-sis-allocator

build/test-hud-frames build/test-particle-cleanup: ../src/melee/gm/types.h
build/test-particle-cleanup: ../src/sysdolphin/baselib/particle.h ../src/sysdolphin/baselib/psstructs.h

build/test-animation-flags: tests/test_animation_flags.c ../src/melee/ft/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fsanitize=address,undefined tests/test_animation_flags.c -o $@
.PHONY: test-animation-flags
test-match-runtime: test-animation-flags
test-animation-flags: build/test-animation-flags
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-animation-flags

# Keep globals separate so the item-table regression cannot pass by BSS adjacency.
build/test-item-spawn-tables: tests/test_item_spawn_tables.c ../src/melee/it/itspawn.c ../src/melee/it/types.h $(NATIVE_HEADERS) | build
	$(CC) $(CPPFLAGS) -I../src -std=gnu11 -O2 -g -fno-common -fsanitize=address,undefined -Wl,-dead_strip tests/test_item_spawn_tables.c -o $@
.PHONY: test-item-spawn-tables
test: test-item-spawn-tables
test-item-spawn-tables: build/test-item-spawn-tables
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-item-spawn-tables
