# metamod/tests/Makefile

COVERAGE_FLAGS ?=
GCOV ?= $(subst g++,gcov,$(firstword $(CXX)))

PARENT_INCLUDES = -I".." -I"../../hlsdk/engine" -I"../../hlsdk/common" \
    -I"../../hlsdk/pm_shared" -I"../../hlsdk/dlls" -I"../../hlsdk"
TEST_CXXFLAGS = ${CXXFLAGS} ${PARENT_INCLUDES} -D__METAMOD_BUILD__ \
    -Wall -Wno-class-memaccess -Wno-write-strings -std=gnu++98 \
    $(COVERAGE_FLAGS)

%.o: %.cpp
	${CXX} ${TEST_CXXFLAGS} -c $< -o $@

%.o: ../%.cpp
	${CXX} ${TEST_CXXFLAGS} -c $< -o $@

# Common objects needed by most tests
COMMON_OBJS = engine_mock.o log_meta.o osdep.o mplayer.o support_meta.o

# Extended common objects for tests that link metamod modules
EXTENDED_OBJS = $(COMMON_OBJS) mreg.o mlist.o mplugin.o conf_meta.o \
    meta_eiface.o sdk_util.o reg_support.o stubs.o

# ============================================================
# Test targets
# ============================================================

test_support_meta: test_support_meta.o $(COMMON_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_osdep: test_osdep.o $(COMMON_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_log_meta: test_log_meta.o $(COMMON_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_mreg: test_mreg.o $(EXTENDED_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_mplayer: test_mplayer.o $(COMMON_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_conf_meta: test_conf_meta.o $(COMMON_OBJS) conf_meta.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_game_support: test_game_support.o $(COMMON_OBJS) game_support.o \
    game_autodetect.o osdep_detect_gamedll_linux.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_game_autodetect: test_game_autodetect.o $(COMMON_OBJS) game_autodetect.o \
    osdep_detect_gamedll_linux.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_engineinfo: test_engineinfo.o $(COMMON_OBJS) engineinfo.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_sdk_util: test_sdk_util.o $(EXTENDED_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_vdate: test_vdate.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^

test_api_hook: test_api_hook.o $(EXTENDED_OBJS) api_hook.o api_info.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_api_info: test_api_info.o $(EXTENDED_OBJS) api_info.o api_hook.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_commands_meta: test_commands_meta.o $(EXTENDED_OBJS) commands_meta.o vdate.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_dllapi: test_dllapi.o $(EXTENDED_OBJS) dllapi.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_engine_api: test_engine_api.o $(EXTENDED_OBJS) engine_api.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_h_export: test_h_export.o $(COMMON_OBJS) meta_eiface.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_linkgame: test_linkgame.o $(COMMON_OBJS) linkgame.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_linkplug: test_linkplug.o $(EXTENDED_OBJS) linkplug.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_meta_eiface: test_meta_eiface.o $(COMMON_OBJS) meta_eiface.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

# metamod.cpp pulls in nearly everything
METAMOD_EXTRA = metamod.o game_support.o game_autodetect.o engineinfo.o \
    commands_meta.o studioapi.o vdate.o api_info.o api_hook.o h_export.o \
    linkgame.o linkplug.o dllapi.o engine_api.o osdep_p.o \
    osdep_linkent_linux.o osdep_detect_gamedll_linux.o mutil.o

test_metamod: test_metamod.o $(EXTENDED_OBJS) $(METAMOD_EXTRA)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_mlist: test_mlist.o $(EXTENDED_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_mplugin: test_mplugin.o $(EXTENDED_OBJS) mutil.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_mutil: test_mutil.o $(EXTENDED_OBJS) mutil.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_reg_support: test_reg_support.o $(EXTENDED_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_studioapi: test_studioapi.o $(COMMON_OBJS) studioapi.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_osdep_p: test_osdep_p.o $(COMMON_OBJS) osdep_p.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_osdep_linkent_linux: test_osdep_linkent_linux.o $(COMMON_OBJS) \
    osdep_linkent_linux.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

test_osdep_detect_gamedll_linux: test_osdep_detect_gamedll_linux.o $(COMMON_OBJS) \
    osdep_detect_gamedll_linux.o
	${CXX} $(COVERAGE_FLAGS) -o $@ $^ -ldl

# ============================================================
# Aggregate targets
# ============================================================

ALL_TESTS = test_support_meta test_osdep test_log_meta test_vdate \
    test_mreg test_mplayer test_conf_meta \
    test_game_support test_game_autodetect test_engineinfo \
    test_sdk_util test_api_hook test_api_info \
    test_commands_meta test_dllapi test_engine_api \
    test_h_export test_linkgame test_linkplug \
    test_meta_eiface test_metamod test_mlist test_mplugin \
    test_mutil test_reg_support test_studioapi \
    test_osdep_p test_osdep_linkent_linux test_osdep_detect_gamedll_linux

all: $(ALL_TESTS)

run: $(ALL_TESTS)
	@for t in $(ALL_TESTS); do ./$$t || exit 1; done

VALGRIND = valgrind --leak-check=full --error-exitcode=1

valgrind: $(ALL_TESTS)
	@for t in $(ALL_TESTS); do $(VALGRIND) ./$$t || exit 1; done

coverage:
	$(MAKE) clean
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage_report
	$(MAKE) COVERAGE_FLAGS="--coverage" run
	lcov --capture --directory . --output-file coverage.info \
	    --gcov-tool $(GCOV) --branch-coverage \
	    --ignore-errors inconsistent,deprecated
	lcov --remove coverage.info '*/tests/*' '*/hlsdk/*' '/usr/*' \
	    --output-file coverage.info --branch-coverage \
	    --ignore-errors inconsistent,deprecated,unused
	genhtml coverage.info --output-directory coverage_report \
	    --branch-coverage --ignore-errors inconsistent,deprecated
	lcov --summary coverage.info --branch-coverage \
	    --ignore-errors inconsistent,deprecated

coverage-clean:
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage_report

clean:
	rm -f $(ALL_TESTS) *.o *.gcda *.gcno *.gcov coverage.info
	rm -rf coverage_report
