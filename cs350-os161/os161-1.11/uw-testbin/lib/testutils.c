
/*
 * Title   : testutils
 * Author  : Tim Brecht
 * Date    : Thu Dec 23 21:19:00 EST 2004
 *
 * Some simple utilities that can be helpful for testing.
 * Feel free to add/modify this code to suit your needs.
 * NOTE: this hasn't been fully tested.
 *
 * See test-testutil, file1 and file2 for example use cases.
 */

#include <stdio.h>
#include "testutils.h"

/* Used to track the number of tests and failures */
static int test_num = 0;
static int num_failures = 0;
static int verbose = 0;

/* Use to print the source file, function and line */
static void
print_location(const char *file, const char *func, int line)
{
  printf("   %s : function = %s, line = %d\n", file, func, line);
}

/* Check if val is equal to expected_val, if not it is an error */
void
test_equal(int val, int expected_val, const char *str, 
            const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val != expected_val) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : expected %d\n", 
      out, test_num, val, expected_val);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line);
  }

} /* test_equal */

/* Check if val is > 0, if not it is an error */
void
test_positive(int val, const char *str,
              const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val <= 0) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : expected Positive Value\n", 
      out, test_num, val);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line);
  }
}

/* Check if val is < 0, if not it is an error */
void
test_negative(int val, const char *str,
              const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val >= 0) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : expected Negative Value\n", 
      out, test_num, val);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line);
  }
}

/* Check if val1 is not equal to val2, if not it is an error */
void
test_not_equal(int val1, int val2, const char *str,
              const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val1 == val2) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : Expected anything but %d\n", 
      out, test_num, val1, val2);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line);
  }
}

/* Reset the statistic counters */
void
test_reset_stats()
{
  num_failures = 0;
  test_num = 0;
}

void
test_verbose_on()
{
  printf("TEST VERBOSE ON\n");
  verbose = 1;
}

void
test_verbose_off()
{
  printf("TEST VERBOSE OFF\n");
  verbose = 0;
}

void
test_print_stats(const char *file, const char* func, int line)
{
  printf("TEST STATS for %s : from function = %s, line = %d\n", 
    file, func, line);
  printf("    Number of failures = %d    Number of Tests = %d\n",
    num_failures, test_num);
  printf("\n");
}
