//
// metamod-p - tests for game_autodetect.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"

#include "engine_mock.h"
#include "test_common.h"

static int test_placeholder(void)
{
	TEST("placeholder - game_autodetect.cpp compiled in test env");

	PASS();
	return 0;
}

int main(void)
{
	int fail = 0;

	mock_reset();

	printf("test_game_autodetect:\n");
	fail |= test_placeholder();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
