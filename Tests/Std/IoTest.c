#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <sys/wait.h>
#include <unistd.h>

// Helper function to check if two strings are equal
static bool StrEqual(const Str* a, const Str* b) {
    if (a->length != b->length) return false;
    for (size_t i = 0; i < a->length; i++) {
        if (a->data[i] != b->data[i]) return false;
    }
    return true;
}

// Helper macro to create TypeSpecificIO array
#define TEST_FMT(...) (TypeSpecificIO[]){__VA_ARGS__}

// Helper function to run a format test in a forked process
static void TestFormat(const char* test_name, const char* fmt, const char* expected, TypeSpecificIO* args, size_t argc) {
    pid_t pid = fork();
    
    if (pid == -1) {
        printf("[ERROR] Failed to fork for test '%s'\n", test_name);
        return;
    }
    
    if (pid == 0) {  // Child process
        Str output = StrInit();
        Str expected_str = StrInitFromZstr(expected);
        bool expect_error = (expected[0] == '\0');  // Empty expected string indicates we expect an error
        
        // Redirect stderr to /dev/null to suppress error messages
        if (expect_error) {
            freopen("/dev/null", "w", stderr);
        }
        
        // Try to format
        StrWriteFmtInternal(&output, fmt, args, argc);
        
        // If we get here and we expected an error, that's a failure
        if (expect_error) {
            printf("[FAIL] Test '%s':\n  Format: %s\n  Expected error but got: %.*s\n",
                   test_name, fmt, (int)output.length, output.data);
            exit(1);
        }
        
        // Check output
        if (!StrEqual(&output, &expected_str)) {
            printf("[FAIL] Test '%s':\n  Format: %s\n  Expected: %.*s\n  Got: %.*s\n",
                   test_name, fmt, 
                   (int)expected_str.length, expected_str.data,
                   (int)output.length, output.data);
            exit(1);
        }
        
        printf("[PASS] Test '%s'\n", test_name);
        StrDeinit(&output);
        StrDeinit(&expected_str);
        exit(0);
    } else {  // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        bool expect_error = (expected[0] == '\0');
        
        if (WIFSIGNALED(status)) {
            // Process was killed by a signal (e.g., SIGABRT)
            if (expect_error) {
                printf("[PASS] Test '%s' (expected error occurred)\n", test_name);
            } else {
                printf("[FAIL] Test '%s': Unexpected crash\n", test_name);
            }
        } else if (WIFEXITED(status)) {
            // Process exited normally
            if (expect_error && WEXITSTATUS(status) == 0) {
                printf("[FAIL] Test '%s': Expected error but completed successfully\n", test_name);
            }
            // Other cases are handled by the child process's output
        }
    }
}

int main(void) {
    // Test integer formatting
    {
        // Test decimal integers
        {
            i8 i8_val = -42;
            i16 i16_val = -1234;
            i32 i32_val = -123456;
            i64 i64_val = -1234567890LL;
            u8 u8_val = 42;
            u16 u16_val = 1234;
            u32 u32_val = 123456;
            u64 u64_val = 1234567890ULL;
            
            TestFormat("i8 decimal", "{}", "-42", TEST_FMT(FMT(i8_val)), 1);
            TestFormat("i16 decimal", "{}", "-1234", TEST_FMT(FMT(i16_val)), 1);
            TestFormat("i32 decimal", "{}", "-123456", TEST_FMT(FMT(i32_val)), 1);
            TestFormat("i64 decimal", "{}", "-1234567890", TEST_FMT(FMT(i64_val)), 1);
            TestFormat("u8 decimal", "{}", "42", TEST_FMT(FMT(u8_val)), 1);
            TestFormat("u16 decimal", "{}", "1234", TEST_FMT(FMT(u16_val)), 1);
            TestFormat("u32 decimal", "{}", "123456", TEST_FMT(FMT(u32_val)), 1);
            TestFormat("u64 decimal", "{}", "1234567890", TEST_FMT(FMT(u64_val)), 1);
        }
        
        // Test hexadecimal formatting
        {
            i32 val = 0xDEADBEEF;
            TestFormat("hex lowercase", "{:x}", "0xdeadbeef", TEST_FMT(FMT(val)), 1);
            TestFormat("hex uppercase", "{:X}", "0xDEADBEEF", TEST_FMT(FMT(val)), 1);
        }
        
        // Test binary formatting
        {
            i8 val = 42;  // 00101010 in binary
            TestFormat("binary", "{:b}", "0b101010", TEST_FMT(FMT(val)), 1);
            
            i8 neg_val = -42;
            TestFormat("binary negative", "{:b}", "-0b101010", TEST_FMT(FMT(neg_val)), 1);
        }
        
        // Test octal formatting
        {
            i32 val = 0755;
            TestFormat("octal", "{:o}", "0o755", TEST_FMT(FMT(val)), 1);
        }
        
        // Test width and alignment
        {
            i32 val = 42;
            TestFormat("right align", "{:>5}", "   42", TEST_FMT(FMT(val)), 1);
            TestFormat("left align", "{:<5}", "42   ", TEST_FMT(FMT(val)), 1);
            TestFormat("center align", "{:^5}", " 42  ", TEST_FMT(FMT(val)), 1);
        }
    }
    
    // Test floating point formatting
    {
        // Test basic float formatting
        {
            f32 f32_val = 3.14159f;
            f64 f64_val = 3.14159265359;
            
            TestFormat("f32 basic", "{}", "3.14159", TEST_FMT(FMT(f32_val)), 1);
            TestFormat("f64 basic", "{}", "3.14159265359", TEST_FMT(FMT(f64_val)), 1);
        }
        
        // Test precision
        {
            f64 val = 3.14159265359;
            TestFormat("float precision 2", "{:.2}", "3.14", TEST_FMT(FMT(val)), 1);
            TestFormat("float precision 4", "{:.4}", "3.1416", TEST_FMT(FMT(val)), 1);
        }
        
        // Test scientific notation
        {
            f64 val = 0.000123;
            TestFormat("scientific lowercase", "{:e}", "1.23e-4", TEST_FMT(FMT(val)), 1);
            TestFormat("scientific uppercase", "{:E}", "1.23E-4", TEST_FMT(FMT(val)), 1);
            
            val = 123456.0;
            TestFormat("scientific large", "{:e}", "1.23456e+5", TEST_FMT(FMT(val)), 1);
        }
        
        // Test width and alignment with floats
        {
            f32 val = 3.14f;
            TestFormat("float right align", "{:>7}", "   3.14", TEST_FMT(FMT(val)), 1);
            TestFormat("float left align", "{:<7}", "3.14   ", TEST_FMT(FMT(val)), 1);
            TestFormat("float center align", "{:^7}", " 3.14  ", TEST_FMT(FMT(val)), 1);
        }
    }
    
    // Test string formatting
    {
        // Test basic string formatting
        {
            Str str = StrInitFromZstr("hello");
            const char* zstr = "world";
            
            TestFormat("Str basic", "{}", "hello", TEST_FMT(FMT(str)), 1);
            TestFormat("zstr basic", "{}", "world", TEST_FMT(FMT(zstr)), 1);
            
            StrDeinit(&str);
        }
        
        // Test width and alignment with strings
        {
            const char* str = "hi";
            TestFormat("string right align", "{:>4}", "  hi", TEST_FMT(FMT(str)), 1);
            TestFormat("string left align", "{:<4}", "hi  ", TEST_FMT(FMT(str)), 1);
            TestFormat("string center align", "{:^4}", " hi ", TEST_FMT(FMT(str)), 1);
        }
    }
    
    // Test positional parameters
    {
        i32 a = 1, b = 2, c = 3;
        TestFormat("positional", "{1} {0} {2}", "2 1 3", 
                  TEST_FMT(FMT(a), FMT(b), FMT(c)), 3);
    }
    
    // Test multiple format specifiers in one string
    {
        i32 num = 42;
        const char* str = "test";
        f64 flt = 3.14;
        
        TestFormat(
            "multiple formats",
            "num={:x} str={} flt={:.2}",
            "num=0x2a str=test flt=3.14",
            TEST_FMT(FMT(num), FMT(str), FMT(flt)),
            3
        );
    }
    
    // Test negative cases
    {
        printf("[INFO] Starting negative test cases - expect error messages below\n");
        
        // Test unmatched braces
        {
            i32 val = 42;
            TestFormat("unmatched open brace", "{", "", TEST_FMT(FMT(val)), 1);
            TestFormat("unmatched close brace", "}", "", TEST_FMT(FMT(val)), 1);
            TestFormat("mismatched braces", "{{}", "", TEST_FMT(FMT(val)), 1);
        }
        
        // Test missing arguments
        {
            i32 val = 42;
            TestFormat("missing argument", "{} {}", "42 ", TEST_FMT(FMT(val)), 1);
        }
        
        // Test invalid format specifiers
        {
            i32 val = 42;
            TestFormat("invalid align", "{:@5}", "", TEST_FMT(FMT(val)), 1);
            TestFormat("invalid type", "{:z}", "", TEST_FMT(FMT(val)), 1);
        }
        
        // Test null pointer handling
        {
            const char* null_str = NULL;
            TestFormat("null string", "{}", "(null)", TEST_FMT(FMT(null_str)), 1);
            
            Str null_str_obj = {0};
            TestFormat("null Str object", "{}", "(null)", TEST_FMT(FMT(null_str_obj)), 1);
        }
        
        // Test invalid precision
        {
            f64 val = 3.14159;
            TestFormat("negative precision", "{:.-2}", "", TEST_FMT(FMT(val)), 1);
            TestFormat("huge precision", "{:.999999}", "", TEST_FMT(FMT(val)), 1);
        }
        
        // Test invalid width
        {
            i32 val = 42;
            TestFormat("negative width", "{:-5}", "", TEST_FMT(FMT(val)), 1);
            TestFormat("huge width", "{:999999}", "", TEST_FMT(FMT(val)), 1);
        }
        
        // Test invalid positional parameters
        {
            i32 a = 1, b = 2;
            TestFormat("invalid position", "{2}", "", TEST_FMT(FMT(a), FMT(b)), 2);
            TestFormat("negative position", "{-1}", "", TEST_FMT(FMT(a)), 1);
            TestFormat("out of bounds", "{999}", "", TEST_FMT(FMT(a)), 1);
        }
        
        // Test mixed valid and invalid cases
        {
            i32 val = 42;
            f64 fval = 3.14;
            TestFormat(
                "mixed valid/invalid",
                "{} {:.2}",
                "42 3.14",
                TEST_FMT(FMT(val), FMT(fval)),
                2
            );
        }
        
        // Test edge cases with strings
        {
            // Empty string
            const char* empty = "";
            TestFormat("empty string", "{:>5}", "     ", TEST_FMT(FMT(empty)), 1);
            
            // Very long string with limited width
            const char* long_str = "this is a very long string that exceeds the specified width";
            TestFormat("long string limited width", "{:10}", "this is a ", TEST_FMT(FMT(long_str)), 1);
        }
        
        // Test floating point edge cases
        {
            f64 inf = 1.0/0.0;  // Infinity
            f64 nan = 0.0/0.0;  // NaN
            TestFormat("infinity", "{}", "inf", TEST_FMT(FMT(inf)), 1);
            TestFormat("nan", "{}", "nan", TEST_FMT(FMT(nan)), 1);
            
            // Very large and small numbers
            f64 very_large = 1e308;
            f64 very_small = 1e-308;
            TestFormat("very large float", "{}", "1e+308", TEST_FMT(FMT(very_large)), 1);
            TestFormat("very small float", "{}", "1e-308", TEST_FMT(FMT(very_small)), 1);
        }
        
        printf("[INFO] Negative test cases completed\n");
    }
    
    printf("[INFO] All format tests completed\n");
    return 0;
} 
