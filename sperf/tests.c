#include <testkit.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// ======================== System Tests ========================

// Test the basic functionality without any arguments
SystemTest(basic_no_args,
           ((const char *[]){}))
{
  tk_assert(result->exit_status == 1,
            "Basic sperf command should exit with status 1, got %d",
            result->exit_status);
  tk_assert(strlen(result->output) > 0,
            "Output should not be empty");
}

// Test the strace 'ls'
SystemTest(ls,
           ((const char *[]){"ls"}))
{
  tk_assert(result->exit_status == 0,
            "sperf ls should exit with status 0, got %d",
            result->exit_status);
  tk_assert(strlen(result->output) > 0,
            "Output should not be empty");
  // Check for presence of PIDs in output (numbers in parentheses)
}