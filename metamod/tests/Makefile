# metamod/tests/Makefile

COVERAGE_FLAGS ?=
SANITIZE_FLAGS ?=
EXTRA_FLAGS = $(COVERAGE_FLAGS) $(SANITIZE_FLAGS)
GCOV ?= $(subst g++,gcov,$(firstword $(CXX)))
TEST_CC = $(subst g++,gcc,$(subst clang++,clang,$(firstword $(CXX))))

PARENT_INCLUDES = -I".." -I"../../hlsdk/engine" -I"../../hlsdk/common" \
    -I"../../hlsdk/pm_shared" -I"../../hlsdk/dlls" -I"../../hlsdk"
TEST_CXXFLAGS = ${CXXFLAGS} ${PARENT_INCLUDES} -D__METAMOD_BUILD__ -DUNITTESTS \
    -Wall -Wno-class-memaccess -Wno-write-strings -Wno-attributes \
    -Wno-format-truncation -fno-strict-aliasing -fno-strict-overflow \
    -std=gnu++98 -march=i686 -msse2 \
    $(EXTRA_FLAGS)

%.o: %.cpp
	${CXX} ${TEST_CXXFLAGS} -MMD -c $< -o $@

%.o: ../%.cpp
	${CXX} ${TEST_CXXFLAGS} -MMD -c $< -o $@

-include $(wildcard *.d)

# Common objects needed by most tests
COMMON_OBJS = engine_mock.o log_meta.o osdep.o mplayer.o support_meta.o

# Extended common objects for tests that link metamod modules
EXTENDED_OBJS = $(COMMON_OBJS) mreg.o mlist.o mplugin.o conf_meta.o \
    meta_eiface.o sdk_util.o reg_support.o stubs.o

# ============================================================
# Test targets
# ============================================================

test_support_meta: test_support_meta.o $(COMMON_OBJS)
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_osdep: test_osdep.o $(COMMON_OBJS)
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_log_meta: test_log_meta.o $(COMMON_OBJS)
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_mreg: test_mreg.o $(EXTENDED_OBJS)
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_mreg_malloc: test_mreg_malloc.o $(filter-out mreg.o,$(EXTENDED_OBJS)) mutil.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_mplayer: test_mplayer.o $(COMMON_OBJS)
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_conf_meta: test_conf_meta.o $(COMMON_OBJS) conf_meta.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_game_support: test_game_support.o $(COMMON_OBJS) game_support.o \
    game_autodetect.o osdep_detect_gamedll_linux.o conf_meta.o fake_gamedll.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl

test_game_autodetect: test_game_autodetect.o $(COMMON_OBJS) game_autodetect.o \
    osdep_detect_gamedll_linux.o fake_gamedll.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl

engine_test.so: fake_engine.c
	$(TEST_CC) -m32 -shared -fPIC -o $@ $<

fake_gamedll.so: fake_gamedll.c
	$(TEST_CC) -m32 -shared -fPIC -o $@ $<

fake_mm_plugin.so: fake_mm_plugin.c
	$(TEST_CC) -m32 -shared -fPIC -o $@ $<

test_engineinfo: test_engineinfo.o $(COMMON_OBJS) engineinfo.o engine_test.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl

test_sdk_util: test_sdk_util.o $(EXTENDED_OBJS)
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_vdate: test_vdate.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^

test_api_hook: test_api_hook.o $(EXTENDED_OBJS) api_hook.o api_info.o dllapi.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl -Wl,--wrap=realloc

test_api_info: test_api_info.o $(EXTENDED_OBJS) api_info.o api_hook.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_commands_meta: test_commands_meta.o $(EXTENDED_OBJS) commands_meta.o vdate.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_dllapi: test_dllapi.o $(EXTENDED_OBJS) dllapi.o api_hook.o api_info.o commands_meta.o vdate.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_engine_api: test_engine_api.o $(EXTENDED_OBJS) engine_api.o api_hook.o api_info.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_h_export: test_h_export.o $(COMMON_OBJS) meta_eiface.o h_export.o \
    engineinfo.o osdep_p.o engine_test.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl

test_linkgame: test_linkgame.o $(COMMON_OBJS) linkgame.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_linkplug: test_linkplug.o $(EXTENDED_OBJS) linkplug.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_meta_eiface: test_meta_eiface.o $(COMMON_OBJS) meta_eiface.o engineinfo.o engine_test.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl

# metamod.cpp pulls in nearly everything
METAMOD_EXTRA = metamod.o game_support.o game_autodetect.o engineinfo.o \
    commands_meta.o studioapi.o vdate.o api_info.o api_hook.o h_export.o \
    linkgame.o linkplug.o dllapi.o engine_api.o osdep_p.o \
    osdep_linkent_linux.o osdep_detect_gamedll_linux.o mutil.o

test_metamod: test_metamod.o $(EXTENDED_OBJS) $(METAMOD_EXTRA)
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_mlist: test_mlist.o $(EXTENDED_OBJS) fake_mm_plugin.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl -Wl,--wrap=realloc

test_mplugin: test_mplugin.o $(EXTENDED_OBJS) mutil.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_metamod_gamedll: test_metamod_gamedll.o $(EXTENDED_OBJS) vdate.o mutil.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_mplugin_lifecycle: test_mplugin_lifecycle.o $(filter-out mplugin.o,$(EXTENDED_OBJS)) mutil.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_mutil: test_mutil.o $(EXTENDED_OBJS) mutil.o fake_gamedll.so fake_mm_plugin.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl

test_reg_support: test_reg_support.o $(EXTENDED_OBJS)
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_studioapi: test_studioapi.o $(COMMON_OBJS) studioapi.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_osdep_p: test_osdep_p.o $(COMMON_OBJS) osdep_p.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_osdep_linkent_linux: test_osdep_linkent_linux.o $(COMMON_OBJS) \
    engine_test.so fake_gamedll.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl -lpthread

test_osdep_detect_gamedll_linux: test_osdep_detect_gamedll_linux.o $(COMMON_OBJS) \
    osdep_detect_gamedll_linux.o fake_gamedll.so fake_mm_plugin.so engine_test.so
	${CXX} $(EXTRA_FLAGS) -o $@ $(filter %.o,$^) -ldl

test_osdep_detect_gamedll_stub: test_osdep_detect_gamedll_stub.o $(COMMON_OBJS) \
    osdep_detect_gamedll_linux.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl -Wl,--wrap=mmap

# Win32 osdep tests (compiled on Linux with stubbed Win32 API)
WIN32_CXXFLAGS = ${TEST_CXXFLAGS} -include win32_stubs.h

osdep_detect_gamedll_win32.o: ../osdep_detect_gamedll_win32.cpp
	${CXX} ${WIN32_CXXFLAGS} -c $< -o $@

osdep_linkent_win32.o: ../osdep_linkent_win32.cpp
	${CXX} ${WIN32_CXXFLAGS} -c $< -o $@

test_osdep_detect_gamedll_win32.o: test_osdep_detect_gamedll_win32.cpp
	${CXX} ${WIN32_CXXFLAGS} -c $< -o $@

test_osdep_linkent_win32.o: test_osdep_linkent_win32.cpp
	${CXX} ${WIN32_CXXFLAGS} -c $< -o $@

test_osdep_detect_gamedll_win32: test_osdep_detect_gamedll_win32.o $(COMMON_OBJS) \
    osdep_detect_gamedll_win32.o win32_stubs.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

test_osdep_linkent_win32: test_osdep_linkent_win32.o $(COMMON_OBJS) \
    osdep_linkent_win32.o win32_stubs.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl -Wl,--wrap=calloc,--wrap=free

# ============================================================
# Aggregate targets
# ============================================================

ALL_TESTS = test_support_meta test_osdep test_log_meta test_vdate \
    test_mreg test_mreg_malloc test_mplayer test_conf_meta \
    test_game_support test_game_autodetect test_engineinfo \
    test_sdk_util test_api_hook test_api_info \
    test_commands_meta test_dllapi test_engine_api \
    test_h_export test_linkgame test_linkplug \
    test_meta_eiface test_metamod test_mlist test_mplugin \
    test_mutil test_reg_support test_studioapi \
    test_osdep_p test_osdep_linkent_linux test_osdep_detect_gamedll_linux \
    test_osdep_detect_gamedll_stub test_mplugin_lifecycle \
    test_metamod_gamedll \
    test_osdep_detect_gamedll_win32 test_osdep_linkent_win32

.PHONY: all run run-opt run-release bench valgrind sanitize clang-sanitize coverage coverage-clean clean

all: $(ALL_TESTS)

run: $(ALL_TESTS)
	@for t in $(ALL_TESTS); do ./$$t || exit 1; done

# The shipped build is optimized. Argument marshalling that folds away only
# once the optimizer can see through a copy is invisible at the default -O0,
# so the suite is also worth running the way the release is built.
run-opt:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="-O2" run

# metamod.so is built with different code generation and a different calling
# convention than the tests default to: PIC, regparm, no frame pointer. Which
# prologue a wrapper gets, and where its arguments live, follow from those, so
# argument marshalling can be right in one build and wrong in the other.
RELEASE_FLAGS = -O2 -fomit-frame-pointer -flto=auto -fPIC -fvisibility=hidden \
    -mtune=generic -fno-exceptions -fno-rtti -D__INTERNALS_USE_REGPARAMS__

run-release:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(RELEASE_FLAGS)" run

# Dispatch cost of the API wrappers, with trivial plugin and gamedll callees
# so what is measured is metamod's own overhead. A live server buries that
# under the ~97% of cycles it spends elsewhere, which is enough to hide a
# regression: pin it to a core and compare against a build of the base branch.
# Built the way metamod.so is: measuring -O0 code would say nothing.
bench:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(RELEASE_FLAGS)" bench_api_hook
	taskset -c 8 ./bench_api_hook

bench_api_hook: bench_api_hook.o $(EXTENDED_OBJS) api_hook.o api_info.o \
    dllapi.o engine_api.o commands_meta.o vdate.o mutil.o
	${CXX} $(EXTRA_FLAGS) -o $@ $^ -ldl

VALGRIND = valgrind --leak-check=full --error-exitcode=1

valgrind: $(ALL_TESTS)
	@for t in $(ALL_TESTS); do $(VALGRIND) ./$$t || exit 1; done

sanitize:
	$(MAKE) clean
	$(MAKE) SANITIZE_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer" run

clang-sanitize:
	$(MAKE) clean
	$(MAKE) SANITIZE_FLAGS="-fsanitize=address,undefined -fno-sanitize=function,enum -fno-sanitize-recover=all -fno-omit-frame-pointer" run

coverage:
	$(MAKE) clean
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage_report
	$(MAKE) COVERAGE_FLAGS="--coverage" run
	lcov --capture --directory . --output-file coverage.info \
	    --gcov-tool $(GCOV) --branch-coverage \
	    --ignore-errors inconsistent,deprecated,mismatch
	lcov --remove coverage.info '*/tests/*' '*/hlsdk/*' '/usr/*' \
	    --output-file coverage.info --branch-coverage \
	    --ignore-errors inconsistent,deprecated,mismatch,unused
	genhtml coverage.info --output-directory coverage_report \
	    --branch-coverage --ignore-errors inconsistent,deprecated,mismatch
	lcov --summary coverage.info --branch-coverage \
	    --ignore-errors inconsistent,deprecated,mismatch

coverage-clean:
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage_report

clean:
	rm -f $(ALL_TESTS) *.o *.d *.gcda *.gcno *.gcov coverage.info \
	    engine_test.so fake_gamedll.so fake_mm_plugin.so
	rm -rf coverage_report
