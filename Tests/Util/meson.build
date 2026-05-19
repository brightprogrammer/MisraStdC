# Build test utilities
test_util_lib = static_library(
  'test_util',
  'TestRunner.c',
  c_args: ['-std=c11'],
  include_directories: inc_misra,
  link_with: [misra_std],
  dependencies: [],
)

# Non-sanitized test utility library for deadend tests
test_util_lib_no_sanitizers = static_library(
  'test_util_no_sanitizers',
  'TestRunner.c',
  c_args: ['-std=c11'],
  include_directories: inc_misra,
  link_with: [misra_std_no_sanitizers],
  dependencies: [],
  override_options: ['b_sanitize=none']
)

# Sanitized test utility paired with misra_std_no_backtrace: deadend-
# heavy suites that need ASan but can't afford per-LOG_FATAL macOS
# dSYM symbolisation use this dep.
test_util_lib_no_backtrace = static_library(
  'test_util_no_backtrace',
  'TestRunner.c',
  c_args: ['-std=c11'],
  include_directories: inc_misra,
  link_with: [misra_std_no_backtrace],
  dependencies: [],
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

# Test utility dependency for sanitised deadend tests that skip
# `LogWrite`'s FATAL backtrace formatter.
test_util_dep_no_backtrace = declare_dependency(
  link_with: test_util_lib_no_backtrace,
  include_directories: ['.'],
)