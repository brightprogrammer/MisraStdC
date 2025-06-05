#include <stdio.h>
#include <Misra/Std/Container/BitVec.h>

int main() {
    BitVec bv = BitVecInit();
    
    // Test remove last element
    BitVecPush(&bv, true);
    printf("Before remove: length=%llu, bit[0]=%d\n", bv.length, BitVecGet(&bv, 0));
    
    bool removed = BitVecRemove(&bv, 0);
    printf("After remove: removed=%d, length=%llu\n", removed, bv.length);
    
    // Test remove from large bitvec
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 3 == 0);
    }
    printf("Large bitvec: length=%llu\n", bv.length);
    
    // Check what bit is at index 500
    bool bit_500 = BitVecGet(&bv, 500);
    bool expected = (500 % 3 == 0);
    printf("Bit at 500: %d, expected: %d\n", bit_500, expected);
    
    // Remove middle element
    removed = BitVecRemove(&bv, 500);
    printf("After remove 500: removed=%d, length=%llu\n", removed, bv.length);
    
    BitVecDeinit(&bv);
    return 0;
} 