#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

// Helper macro to create TypeSpecificIO array
#define TEST_FMT(...) (TypeSpecificIO[]){__VA_ARGS__}

// Helper function to run a read test in a forked process
static void TestRead(const char* test_name, const char* input, const char* fmt, TypeSpecificIO* args, size argc, bool expect_error) {
    pid_t pid = fork();
    
    if (pid == -1) {
        printf("[ERROR] Failed to fork for test '%s'\n", test_name);
        return;
    }
    
    if (pid == 0) {  // Child process
        // Redirect stderr to /dev/null to suppress error messages if we expect an error
        if (expect_error) {
            freopen("/dev/null", "w", stderr);
        }
        
        // Try to read
        const char* result = StrReadFmtInternal(input, fmt, args, argc);
        
        if (expect_error) {
            if (result != NULL) {
                printf("[FAIL] Test '%s':\n  Input: %s\n  Format: %s\n  Expected error but succeeded\n",
                       test_name, input, fmt);
                exit(1);
            }
            printf("[PASS] Test '%s' (expected error occurred)\n", test_name);
        } else {
            if (result == NULL) {
                printf("[FAIL] Test '%s':\n  Input: %s\n  Format: %s\n  Failed to parse\n",
                       test_name, input, fmt);
                exit(1);
            }
            
            // Skip whitespace
            while (*result && isspace(*result)) result++;
            
            if (*result != '\0') {
                printf("[FAIL] Test '%s':\n  Input: %s\n  Format: %s\n  Unconsumed input: %s\n",
                       test_name, input, fmt, result);
                exit(1);
            }
            
            printf("[PASS] Test '%s'\n", test_name);
        }
        exit(0);
    } else {  // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFSIGNALED(status)) {
            // Process was killed by a signal (e.g., SIGABRT)
            if (expect_error) {
                printf("[PASS] Test '%s' (expected error occurred)\n", test_name);
            } else {
                printf("[FAIL] Test '%s': Unexpected crash\n", test_name);
            }
        }
    }
}

int main(void) {
    printf("[INFO] Starting format reader tests\n");
    
    // ===== INTEGER READING TESTS =====
    // Tests reading integers of different types, bases, and edge cases
    {
        // Test decimal integers
        {
            i8 i8_val;
            i16 i16_val;
            i32 i32_val;
            i64 i64_val;
            u8 u8_val;
            u16 u16_val;
            u32 u32_val;
            u64 u64_val;
            
            TestRead("i8 decimal", "-42", "{}", TEST_FMT(FMT(i8_val)), 1, false);
            TestRead("i16 decimal", "-1234", "{}", TEST_FMT(FMT(i16_val)), 1, false);
            TestRead("i32 decimal", "-123456", "{}", TEST_FMT(FMT(i32_val)), 1, false);
            TestRead("i64 decimal", "-1234567890", "{}", TEST_FMT(FMT(i64_val)), 1, false);
            TestRead("u8 decimal", "42", "{}", TEST_FMT(FMT(u8_val)), 1, false);
            TestRead("u16 decimal", "1234", "{}", TEST_FMT(FMT(u16_val)), 1, false);
            TestRead("u32 decimal", "123456", "{}", TEST_FMT(FMT(u32_val)), 1, false);
            TestRead("u64 decimal", "1234567890", "{}", TEST_FMT(FMT(u64_val)), 1, false);
            
            // Test edge cases for integer parsing
            TestRead("i8 max", "127", "{}", TEST_FMT(FMT(i8_val)), 1, false);
            TestRead("i8 min", "-127", "{}", TEST_FMT(FMT(i8_val)), 1, false);
            TestRead("u8 max", "255", "{}", TEST_FMT(FMT(u8_val)), 1, false);
            TestRead("u8 min", "0", "{}", TEST_FMT(FMT(u8_val)), 1, false);
            
            // Test leading zeros
            TestRead("leading zeros", "000042", "{}", TEST_FMT(FMT(i32_val)), 1, false);
            TestRead("negative leading zeros", "-000042", "{}", TEST_FMT(FMT(i32_val)), 1, false);
            
            // Test whitespace handling
            TestRead("leading whitespace", "   42", "{}", TEST_FMT(FMT(i32_val)), 1, false);
            TestRead("trailing whitespace", "42   ", "{}", TEST_FMT(FMT(i32_val)), 1, false);
            TestRead("mixed whitespace", "  42  ", "{}", TEST_FMT(FMT(i32_val)), 1, false);
        }
        
        // Test hexadecimal reading
        {
            u32 val;
            TestRead("hex lowercase", "0xdeadbeef", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("hex uppercase", "0xDEADBEEF", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("hex without prefix", "deadbeef", "{}", TEST_FMT(FMT(val)), 1, false);
            
            // Test hex edge cases
            TestRead("hex zero", "0x0", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("hex one char", "0xf", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("hex mixed case", "0xaBcDeF", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("hex invalid char", "0xGHIJ", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("hex incomplete", "0x", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("hex with spaces", "0x dead beef", "{}", TEST_FMT(FMT(val)), 1, true);
        }
        
        // Test binary reading
        {
            i8 val;
            TestRead("binary with prefix", "0b101010", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("binary without prefix", "101010", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("binary negative", "-0b101010", "{}", TEST_FMT(FMT(val)), 1, false);
            
            // Test binary edge cases
            TestRead("binary zero", "0b0", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("binary one", "0b1", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("binary invalid digit", "0b102", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("binary incomplete", "0b", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("binary with spaces", "0b1010 1010", "{}", TEST_FMT(FMT(val)), 1, true);
        }
        
        // Test octal reading
        {
            i32 val;
            TestRead("octal with prefix", "0o755", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("octal without prefix", "755", "{}", TEST_FMT(FMT(val)), 1, false);
            
            // Test octal edge cases
            TestRead("octal zero", "0o0", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("octal one digit", "0o7", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("octal invalid digit", "0o8", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("octal incomplete", "0o", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("octal with spaces", "0o7 5 5", "{}", TEST_FMT(FMT(val)), 1, true);
        }
        
        // Test integer overflow/underflow
        {
            i8 i8_val;
            u8 u8_val;
            TestRead("i8 overflow", "128", "{}", TEST_FMT(FMT(i8_val)), 1, true);
            TestRead("i8 underflow", "-129", "{}", TEST_FMT(FMT(i8_val)), 1, true);
            TestRead("u8 overflow", "256", "{}", TEST_FMT(FMT(u8_val)), 1, true);
            TestRead("u8 negative", "-1", "{}", TEST_FMT(FMT(u8_val)), 1, true);
        }
    }
    
    // Test floating point reading
    {
        // Test basic float reading
        {
            f32 f32_val;
            f64 f64_val;
            
            TestRead("f32 basic", "3.14159", "{}", TEST_FMT(FMT(f32_val)), 1, false);
            TestRead("f64 basic", "3.14159265359", "{}", TEST_FMT(FMT(f64_val)), 1, false);
            
            // Test float edge cases
            TestRead("float zero", "0.0", "{}", TEST_FMT(FMT(f64_val)), 1, false);
            TestRead("float negative zero", "-0.0", "{}", TEST_FMT(FMT(f64_val)), 1, false);
            TestRead("float no decimal", "42.", "{}", TEST_FMT(FMT(f64_val)), 1, false);
            TestRead("float no integer", ".42", "{}", TEST_FMT(FMT(f64_val)), 1, false);
            TestRead("float multiple dots", "1.2.3", "{}", TEST_FMT(FMT(f64_val)), 1, true);
            TestRead("float invalid chars", "1.2x3", "{}", TEST_FMT(FMT(f64_val)), 1, true);
        }
        
        // Test scientific notation
        {
            f64 val;
            TestRead("scientific lowercase", "1.23e-4", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("scientific uppercase", "1.23E-4", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("scientific positive exp", "1.23e+5", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("scientific no exp sign", "1.23e5", "{}", TEST_FMT(FMT(val)), 1, false);
            
            // Test scientific notation edge cases
            TestRead("scientific zero exp", "1.23e0", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("scientific negative zero", "-0.0e0", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("scientific no decimal", "1e5", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("scientific no integer", ".1e5", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("scientific incomplete exp", "1.23e", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("scientific invalid exp", "1.23ef", "{}", TEST_FMT(FMT(val)), 1, true);
        }
        
        // Test special values
        {
            f64 val;
            TestRead("infinity", "inf", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("negative infinity", "-inf", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("nan", "nan", "{}", TEST_FMT(FMT(val)), 1, false);
            
            // Test special value variations
            TestRead("infinity uppercase", "INF", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("infinity mixed case", "InF", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("nan uppercase", "NAN", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("nan mixed case", "NaN", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("invalid special", "infinite", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("invalid nan", "nana", "{}", TEST_FMT(FMT(val)), 1, true);
        }
        
        // Test edge cases
        {
            f64 val;
            TestRead("very large float", "1e308", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("very small float", "1e-308", "{}", TEST_FMT(FMT(val)), 1, false);
            TestRead("float overflow", "1e309", "{}", TEST_FMT(FMT(val)), 1, true);
            TestRead("float underflow", "1e-324", "{}", TEST_FMT(FMT(val)), 1, true);
        }
    }
    
    // Test string reading
    {
        // Test basic string reading
        {
            Str str = StrInit();
            TestRead("Str basic", "hello", "{}", TEST_FMT(FMT(str)), 1, false);
            StrDeinit(&str);

            const char* zstr = NULL;
            TestRead("zstr basic", "world", "{}", TEST_FMT(FMT(zstr)), 1, false);
            free((void*)zstr);
            
            // Test string with special characters
            TestRead("string with numbers", "hello123", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("string with symbols", "hello!@#", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("string with spaces", "\"hello world\"", "{}", TEST_FMT(FMT(str)), 1, false);
        }
        
        // Test quoted strings
        {
            Str str = StrInit();
            TestRead("quoted string", "\"hello world\"", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("single quoted", "'hello world'", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("escaped quotes", "\"hello \\\"world\\\"\"", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("mixed quotes", "\"hello' world\"", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("nested quotes", "'\"hello world\"'", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("escaped single quote", "'hello\\'world'", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("mismatched quotes", "'hello\"", "{}", TEST_FMT(FMT(str)), 1, true);
            TestRead("unclosed escape", "'hello\\", "{}", TEST_FMT(FMT(str)), 1, true);
            StrDeinit(&str);
        }
        
        // Test string edge cases
        {
            Str str = StrInit();
            TestRead("empty string", "\"\"", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("whitespace string", "\"   \"", "{}", TEST_FMT(FMT(str)), 1, false);
            TestRead("unmatched quote", "\"hello", "{}", TEST_FMT(FMT(str)), 1, true);
            StrDeinit(&str);
        }
    }
    
    // Test escape sequences
    {
        Str str = StrInit();
        TestRead("escape newline", "\"hello\\nworld\"", "{}", TEST_FMT(FMT(str)), 1, false);
        TestRead("escape tab", "\"hello\\tworld\"", "{}", TEST_FMT(FMT(str)), 1, false);
        TestRead("escape backslash", "\"hello\\\\world\"", "{}", TEST_FMT(FMT(str)), 1, false);
        TestRead("escape hex", "\"hello\\x20world\"", "{}", TEST_FMT(FMT(str)), 1, false);
        TestRead("invalid escape", "\"hello\\zworld\"", "{}", TEST_FMT(FMT(str)), 1, true);
        TestRead("incomplete hex escape", "\"hello\\x2g\"", "{}", TEST_FMT(FMT(str)), 1, true);
        StrDeinit(&str);
    }
    
    // Test multiple values
    {
        i32 a = 0, b = 0, c = 0;
        TestRead("multiple integers", "1 2 3", "{} {} {}", 
                TEST_FMT(FMT(a), FMT(b), FMT(c)), 3, false);
        
        f64 x = 0.0;
        Str str = StrInit();
        i32 val = 0;
        TestRead("mixed types", "3.14 hello 42", "{} {} {}", 
                TEST_FMT(FMT(x), FMT(str), FMT(val)), 3, false);
        StrDeinit(&str);
        
        // Test multiple value edge cases
        i32 nums[3];
        TestRead("multiple zeros", "0 0 0", "{} {} {}", 
                TEST_FMT(FMT(nums[0]), FMT(nums[1]), FMT(nums[2])), 3, false);
        
        TestRead("multiple whitespace", "1     2     3", "{} {} {}", 
                TEST_FMT(FMT(nums[0]), FMT(nums[1]), FMT(nums[2])), 3, false);
        
        TestRead("mixed number bases", "42 0xFF 0b1010", "{} {} {}", 
                TEST_FMT(FMT(nums[0]), FMT(nums[1]), FMT(nums[2])), 3, false);
        
        // Test mixed type edge cases
        f64 f;
        Str s = StrInit();
        i32 i;
        TestRead("special float mix", "inf \"hello\" 42", "{} {} {}", 
                TEST_FMT(FMT(f), FMT(s), FMT(i)), 3, false);
        
        TestRead("escaped string mix", "3.14 \"hello\\nworld\" -42", "{} {} {}", 
                TEST_FMT(FMT(f), FMT(s), FMT(i)), 3, false);
        
        TestRead("scientific mix", "1e-10 \"test\" 0xFF", "{} {} {}", 
                TEST_FMT(FMT(f), FMT(s), FMT(i)), 3, false);
        StrDeinit(&s);
    }
    
    // Test error cases
    {
        i32 val;
        TestRead("invalid integer", "xyz", "{}", TEST_FMT(FMT(val)), 1, true);  // xyz is not a valid hex number
        TestRead("missing value", "", "{}", TEST_FMT(FMT(val)), 1, true);
        TestRead("incomplete format", "42", "{} {}", TEST_FMT(FMT(val)), 1, true);
        TestRead("extra input", "42 extra", "{}", TEST_FMT(FMT(val)), 1, true);
        
        // Additional error cases
        TestRead("only whitespace", "   ", "{}", TEST_FMT(FMT(val)), 1, true);
        TestRead("invalid chars", "@#$", "{}", TEST_FMT(FMT(val)), 1, true);
        TestRead("mixed invalid", "42abc", "{}", TEST_FMT(FMT(val)), 1, true);
        TestRead("wrong order", "hello 42", "{}", TEST_FMT(FMT(val)), 1, true);
        
        // Test format string errors
        TestRead("empty format", "", "{}", TEST_FMT(FMT(val)), 1, true);
        TestRead("invalid format", "42", "{:}", TEST_FMT(FMT(val)), 1, true);
        TestRead("mismatched format count", "42 43", "{}", TEST_FMT(FMT(val)), 1, true);
    }
    
    // Test structured input formats
    {
        printf("[INFO] Testing structured input formats\n");
        
        // Simple structure with unsigned integer
        {
            u32 val = 0;
            const char* input = "Value: 42";
            const char* fmt = "Value: {}";
            
            TestRead("simple structure", input, fmt, TEST_FMT(FMT(val)), 1, false);
        }
        
        // Simple structure with floating point
        {
            f64 val = 0.0;
            const char* input = "Price: 19.99";
            const char* fmt = "Price: {}";
            
            TestRead("simple float structure", input, fmt, TEST_FMT(FMT(val)), 1, false);
        }
        
        // Simple structure with string
        {
            Str val = StrInit();
            const char* input = "Name: Alice";
            const char* fmt = "Name: {}";
            
            TestRead("simple string structure", input, fmt, TEST_FMT(FMT(val)), 1, false);
            StrDeinit(&val);
        }
        
        // Structure with two placeholders
        // Note: When using multiple placeholders, separate them with whitespace around punctuation
        // to avoid parsing issues.
        {
            u32 quantity = 0;
            f64 price = 0.0;
            
            const char* input = "Quantity: 5 , Price: 19.99";
            const char* fmt = "Quantity: {} , Price: {}";
            
            TestRead("two-field structure", input, fmt, 
                    TEST_FMT(FMT(quantity), FMT(price)), 2, false);
        }
        
        // Error cases
        {
            u32 val = 0;
            
            // Format doesn't match input
            TestRead("mismatched format", "Count: 42", "Value: {}", 
                    TEST_FMT(FMT(val)), 1, true);
                    
            // Extra content in input
            TestRead("extra input", "Value: 42 extra", "Value: {}", 
                    TEST_FMT(FMT(val)), 1, true);
        }
    }
    
    printf("[INFO] All format reader tests completed\n");
    return 0;
} 
