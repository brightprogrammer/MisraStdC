# Build test utilities
test_util_lib = static_library(
  'test_util',
  'TestRunner.c',
  c_args: ['-std=c11'],
  dependencies: [],
)

# Test utility dependency for other tests to use
test_util_dep = declare_dependency(
  link_with: test_util_lib,
  include_directories: ['.'],
) 