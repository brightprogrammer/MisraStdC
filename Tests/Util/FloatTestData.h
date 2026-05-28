/// file      : Tests/Util/FloatTestData.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Long-digit-run string literals shared by the Float.* test suites.
/// Each macro expands to a string of repeated digits used to stress
/// the Float parser's high-precision paths. No code; header-only data.

#ifndef MISRA_TEST_FLOAT_DATA_H
#define MISRA_TEST_FLOAT_DATA_H

#define FLOAT_TEST_DIGITS_1x50                                                                                         \
    "1111111111"                                                                                                       \
    "1111111111"                                                                                                       \
    "1111111111"                                                                                                       \
    "1111111111"                                                                                                       \
    "1111111111"
#define FLOAT_TEST_DIGITS_2x50                                                                                         \
    "2222222222"                                                                                                       \
    "2222222222"                                                                                                       \
    "2222222222"                                                                                                       \
    "2222222222"                                                                                                       \
    "2222222222"
#define FLOAT_TEST_DIGITS_3x50                                                                                         \
    "3333333333"                                                                                                       \
    "3333333333"                                                                                                       \
    "3333333333"                                                                                                       \
    "3333333333"                                                                                                       \
    "3333333333"

#define FLOAT_TEST_DIGITS_1x100 FLOAT_TEST_DIGITS_1x50 FLOAT_TEST_DIGITS_1x50
#define FLOAT_TEST_DIGITS_2x100 FLOAT_TEST_DIGITS_2x50 FLOAT_TEST_DIGITS_2x50
#define FLOAT_TEST_DIGITS_3x100 FLOAT_TEST_DIGITS_3x50 FLOAT_TEST_DIGITS_3x50

#define FLOAT_TEST_VERY_LARGE_ONES   FLOAT_TEST_DIGITS_1x100 "." FLOAT_TEST_DIGITS_1x100
#define FLOAT_TEST_VERY_LARGE_TWOS   FLOAT_TEST_DIGITS_2x100 "." FLOAT_TEST_DIGITS_2x100
#define FLOAT_TEST_VERY_LARGE_THREES FLOAT_TEST_DIGITS_3x100 "." FLOAT_TEST_DIGITS_3x100

#endif // MISRA_TEST_FLOAT_DATA_H
