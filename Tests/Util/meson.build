# Build test utilities
test_util_lib = static_library(
  'test_util',
  'TestRunner.c',
  c_args: ['-std=c11'],
  include_directories: inc_misra,
  link_with: [misra_std],
  dependencies: [],
)

# Test utility dependency for other tests to use
test_util_dep = declare_dependency(
  link_with: test_util_lib,
  include_directories: ['.'],
) 