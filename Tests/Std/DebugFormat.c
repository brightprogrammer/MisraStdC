#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <stdio.h>

int main(void) {
    i32 val = -42;
    
    printf("Testing formatting of -42 in different bases\n");
    
    // Test decimal format
    Str dec = StrInit();
    TypeSpecificIO io = FMT(val);
    StrWriteFmtInternal(&dec, "{}", &io, 1);
    printf("Decimal: %s\n", dec.data);
    StrDeinit(&dec);
    
    // Test hex format
    Str hex = StrInit();
    FmtInfo hex_fmt = {
        .align = ALIGN_RIGHT,
        .width = 0,
        .precision = 6,
        .has_precision = false,
        .is_hex = true,
        .is_binary = false,
        .is_octal = false,
        .is_debug = false,
        .is_scientific = false,
        .is_caps = false
    };
    i64 hex_val = val; // Convert to i64 to match what _write_i32 does
    _write_i64(&hex, &hex_fmt, &hex_val);
    printf("Hex: %s\n", hex.data);
    StrDeinit(&hex);
    
    // Test binary format
    Str bin = StrInit();
    FmtInfo bin_fmt = {
        .align = ALIGN_RIGHT,
        .width = 0,
        .precision = 6,
        .has_precision = false,
        .is_hex = false,
        .is_binary = true,
        .is_octal = false,
        .is_debug = false,
        .is_scientific = false,
        .is_caps = false
    };
    i64 bin_val = val; // Convert to i64 to match what _write_i32 does
    _write_i64(&bin, &bin_fmt, &bin_val);
    printf("Binary: %s\n", bin.data);
    StrDeinit(&bin);
    
    // Test octal format
    Str oct = StrInit();
    FmtInfo oct_fmt = {
        .align = ALIGN_RIGHT,
        .width = 0,
        .precision = 6,
        .has_precision = false,
        .is_hex = false,
        .is_binary = false,
        .is_octal = true,
        .is_debug = false,
        .is_scientific = false,
        .is_caps = false
    };
    i64 oct_val = val; // Convert to i64 to match what _write_i32 does
    _write_i64(&oct, &oct_fmt, &oct_val);
    printf("Octal: %s\n", oct.data);
    StrDeinit(&oct);
    
    return 0;
} 

