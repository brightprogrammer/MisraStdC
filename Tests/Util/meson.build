# Build test utilities
test_util_lib = static_library(
  'test_util',
  'TestRunner.c',
  include_directories: inc_misra,
  link_with: [misra_std],
  dependencies: [],
  override_options: [
    'c_std=c11'
  ]
)

# Non-sanitized test utility library for deadend tests
test_util_lib_no_sanitizers = static_library(
  'test_util_no_sanitizers',
  'TestRunner.c',
  include_directories: inc_misra,
  link_with: [misra_std_no_sanitizers],
  dependencies: [],
  override_options: [
    'b_sanitize=none',
    'c_std=c11'
  ]
)

# Test utility dependency for normal tests to use
test_util_dep = declare_dependency(
  link_with: test_util_lib,
  include_directories: ['.'],
)

# Test utility dependency for deadend tests to use
test_util_dep_no_sanitizers = declare_dependency(
  link_with: test_util_lib_no_sanitizers,
  include_directories: ['.'],
) 

