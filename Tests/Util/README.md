# Test Utilities

This directory contains utilities for testing potentially failing or crashing functions safely.

## test_deadend Function

The `test_deadend` function allows you to run test functions in separate processes, which is useful for testing operations that might crash, abort, or fail in ways that would normally terminate the test process.

### Usage

```c
#include "../Util/TestRunner.h"

// Your test function that might fail or crash
bool test_that_might_crash(void) {
    // Some potentially dangerous operation
    int* null_ptr = NULL;
    *null_ptr = 42;  // This will crash
    return true;     // Never reached
}

int main(void) {
    // Test a function that should crash, and we expect it to fail
    if (test_deadend(test_that_might_crash, true)) {
        printf("PASS - Test crashed as expected\n");
    } else {
        printf("FAIL - Test didn't crash as expected\n");
    }
    
    return 0;
}
```

### Function Signature

```c
bool test_deadend(TestFunction test_func, bool expect_failure);
```

**Parameters:**
- `test_func`: A function pointer to the test function to execute
- `expect_failure`: 
  - `true` if you expect the test to fail (non-zero exit code, crash, abort, etc.)
  - `false` if you expect the test to succeed (zero exit code)

**Returns:**
- `true` if the test behaved as expected
- `false` if the test didn't behave as expected

### How It Works

#### On Unix/Linux/macOS
- Uses `fork()` to create a child process
- Runs the test function in the child process
- Parent process waits for child and checks exit status
- Handles normal exits, signal terminations, and other failure modes

#### On Windows
- Uses structured exception handling (`__try`/`__except`)
- Catches crashes and exceptions within the same process
- Less isolation than Unix version but still provides crash protection

### Example Scenarios

#### Testing Expected Failures

```c
// Test that should fail due to invalid operation
bool test_invalid_operation(void) {
    // Some operation that should fail
    return false;  // Indicates failure
}

// Test it with test_deadend expecting failure
test_deadend(test_invalid_operation, true);  // Should return true
```

#### Testing Expected Crashes

```c
// Test that should crash
bool test_segfault(void) {
    int* p = NULL;
    *p = 42;  // Segmentation fault
    return true;  // Never reached
}

// Test it with test_deadend expecting failure
test_deadend(test_segfault, true);  // Should return true (crash detected)
```

#### Testing Expected Success

```c
// Test that should pass
bool test_normal_operation(void) {
    int x = 2 + 2;
    return x == 4;  // Should return true
}

// Test it with test_deadend expecting success
test_deadend(test_normal_operation, false);  // Should return true
```

### Integration with Meson Build System

To use test_deadend in your test files, you need to link with the test utilities:

```meson
# In your Tests/meson.build file
test_exe = executable(
    'YourTest',
    files('YourTest.c'),
    link_with: [misra_std],
    dependencies: [test_util_dep],  # Add this line
    c_args: test_args,
    include_directories: inc_misra,
    install: false
)
```

### Real-World Example

See `Tests/Std/Vec.Remove.c` for a real example of how test_deadend is used to test potentially failing vector operations:

```c
// Test that tries to access an invalid index
bool test_invalid_index_access(void) {
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();
    
    int val1 = 42, val2 = 43;
    VecPushBack(&vec, val1);
    VecPushBack(&vec, val2);
    
    // This should fail or crash due to out-of-bounds access
    int invalid_value = VecAt(&vec, 100);
    
    VecDeinit(&vec);
    return invalid_value == 42;
}

// In main():
if (test_deadend(test_invalid_index_access, true)) {
    printf("PASS - Invalid access failed as expected\n");
} else {
    printf("FAIL - Invalid access should have failed\n");
}
```

### Benefits

1. **Safety**: Test potentially crashing code without terminating the test suite
2. **Isolation**: Each dangerous test runs in its own process
3. **Comprehensive Testing**: Test both success and failure scenarios
4. **Cross-Platform**: Works on both Unix-like systems and Windows
5. **Easy Integration**: Simple function call interface

### Limitations

- **Windows**: Less process isolation compared to Unix systems
- **Performance**: Creating processes has overhead
- **Debugging**: Harder to debug code running in child processes
- **Platform Differences**: Behavior may vary slightly between platforms

Use test_deadend when you need to test operations that might crash, abort, or fail in ways that would normally terminate your test process. 
