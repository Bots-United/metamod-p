//
// metamod-p - common test framework macros
//

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <string.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
   tests_run++; \
   printf("  %-60s ", name); \
} while(0)

#define PASS() do { \
   tests_passed++; \
   printf("OK\n"); \
} while(0)

#define ASSERT_STR(actual, expected) do { \
   if (strcmp((actual), (expected)) != 0) { \
      printf("FAIL\n    expected: \"%s\"\n    got:      \"%s\"\n", \
             (expected), (actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_INT(actual, expected) do { \
   if ((actual) != (expected)) { \
      printf("FAIL\n    expected: %d\n    got:      %d\n", \
             (expected), (actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_PTR_EQ(actual, expected) do { \
   if ((actual) != (expected)) { \
      printf("FAIL\n    expected: %p\n    got:      %p\n", \
             (void*)(expected), (void*)(actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_PTR_NULL(actual) do { \
   if ((actual) != NULL) { \
      printf("FAIL\n    expected: NULL\n    got:      %p\n", \
             (void*)(actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_PTR_NOT_NULL(actual) do { \
   if ((actual) == NULL) { \
      printf("FAIL\n    expected: non-NULL\n    got:      NULL\n"); \
      return 1; \
   } \
} while(0)

#define ASSERT_TRUE(cond) do { \
   if (!(cond)) { \
      printf("FAIL\n    condition false: %s\n", #cond); \
      return 1; \
   } \
} while(0)

#define ASSERT_FALSE(cond) do { \
   if ((cond)) { \
      printf("FAIL\n    condition should be false: %s\n", #cond); \
      return 1; \
   } \
} while(0)

#define ASSERT_FLOAT_NEAR(actual, expected, eps) do { \
   float _a = (actual), _e = (expected), _eps = (eps); \
   if (fabs(_a - _e) > _eps) { \
      printf("FAIL\n    expected: %f (+/- %f)\n    got:      %f\n", \
             (double)_e, (double)_eps, (double)_a); \
      return 1; \
   } \
} while(0)

#ifndef EPSILON
#define EPSILON 1e-4f
#endif

#define ASSERT_FLOAT(actual, expected) do { \
   float _a = (actual), _e = (expected); \
   if (fabsf(_a - _e) > EPSILON) { \
      printf("FAIL\n    expected: %f\n    got:      %f\n", \
             (double)_e, (double)_a); \
      return 1; \
   } \
} while(0)

#define ASSERT_STR_CONTAINS(haystack, needle) do { \
   const char *_h = (haystack), *_n = (needle); \
   if (!_h || !strstr(_h, _n)) { \
      printf("FAIL\n    string: \"%s\"\n    missing: \"%s\"\n", \
             _h ? _h : "(null)", _n); \
      return 1; \
   } \
} while(0)

#define ASSERT_SHORT(actual, expected) do { \
   short _a = (actual), _e = (expected); \
   if (_a != _e) { \
      printf("FAIL\n    expected: %d\n    got:      %d\n", \
             (int)_e, (int)_a); \
      return 1; \
   } \
} while(0)

#define ASSERT_USHORT(actual, expected) do { \
   unsigned short _a = (actual), _e = (expected); \
   if (_a != _e) { \
      printf("FAIL\n    expected: %u\n    got:      %u\n", \
             (unsigned)_e, (unsigned)_a); \
      return 1; \
   } \
} while(0)

// Temp file helper for tests that need config file I/O
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

static char _test_tmp_path[512];

__attribute__((unused))
static const char *make_tmp_file(const char *content)
{
   snprintf(_test_tmp_path, sizeof(_test_tmp_path),
            "/tmp/metamod_test_XXXXXX");
   int fd = mkstemp(_test_tmp_path);
   if (fd < 0) return NULL;
   if (content) {
      size_t len = strlen(content);
      ssize_t n = write(fd, content, len);
      (void)n;
   }
   close(fd);
   return _test_tmp_path;
}

__attribute__((unused))
static void cleanup_tmp_file(void)
{
   if (_test_tmp_path[0])
      unlink(_test_tmp_path);
   _test_tmp_path[0] = '\0';
}

static char _test_tmp_path2[512];

__attribute__((unused))
static const char *make_tmp_file_in(const char *content, const char *dir)
{
   snprintf(_test_tmp_path2, sizeof(_test_tmp_path2),
            "%s/metamod_test_XXXXXX", dir);
   int fd = mkstemp(_test_tmp_path2);
   if (fd < 0) return NULL;
   if (content) {
      size_t len = strlen(content);
      ssize_t n = write(fd, content, len);
      (void)n;
   }
   close(fd);
   return _test_tmp_path2;
}

// Copy a test .so file to a destination path, using __FILE__ to locate it
// regardless of CWD.
__attribute__((unused))
static void copy_test_plugin(const char *so_name, const char *dest)
{
   char cmd[PATH_MAX * 2];
   char srcdir[PATH_MAX];
   strncpy(srcdir, __FILE__, sizeof(srcdir) - 1);
   srcdir[sizeof(srcdir) - 1] = '\0';
   char *slash = strrchr(srcdir, '/');
   if (slash) *(slash + 1) = '\0';
   else strcpy(srcdir, "./");
   snprintf(cmd, sizeof(cmd), "cp %s%s %s", srcdir, so_name, dest);
   system(cmd);
}

#endif // TEST_COMMON_H
