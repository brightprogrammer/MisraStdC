/// file      : std/container/vec/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic vector type definition

#ifndef MISRA_STD_CONTAINER_VEC_TYPE_H
#define MISRA_STD_CONTAINER_VEC_TYPE_H

#include <Misra/Std/Container/Common.h>
#include <Misra/Types.h>
#include <Misra/Std/Log.h>

typedef struct {
    size              length;
    size              capacity;
    GenericCopyInit   copy_init;
    GenericCopyDeinit copy_deinit;
    char             *data;
    size              alignment;
} GenericVec;

#define GENERIC_VEC(x) ((GenericVec *)(void *)(x))

///
/// Typesafe vector definition.
/// This is much like C++ template std::vector<T>
///
/// NOTE: Using this directly like `Vec(T)` won't always work,
///       because each time this is used it defines a new type, and two
///       `Vec(T)`s are different from each other.
///
///       To deal with this, you must typedef vector for a specific type.
///       Throughout the code, any time that is in plural form is generally
///       a vector. Like `Strs` is a typedef of `Vec(Str)`.
///
/// USAGE:
///   Vec(int) integers; // Vector of integers
///   Vec(CustomStruct) my_data; // Vector of CustomStruct
///   Vec(float) real_numbers; // Vector of float values
///   Vec(const char*) names; Vector of c-style null-terminated strings
///
/// FIELDS:
/// - length      : Number of items currently in vector (always <= capacity)
/// - capacity    : Max number of items this vector can hold before doing a resize.
/// - copy_init   : If provided then is used to create owned copies of items into vector.
/// - copy_deinit : If provided then is used to deinit data held by vector.
///                 Caution when dealing with shared ownership.
/// - data        : Data held by vector. Don't access by direct indexing. Use `VecAt(..)`
/// - alignment   : Alignment requirement for each item in vector.
///
/// TAGS: Vec, Generic, Length, Size, Aligned, Pointer
///
#define Vec(T)                                                                                                         \
    struct {                                                                                                           \
        size              length;                                                                                      \
        size              capacity;                                                                                    \
        GenericCopyInit   copy_init;                                                                                   \
        GenericCopyDeinit copy_deinit;                                                                                 \
        T                *data;                                                                                        \
        size              alignment;                                                                                   \
    }

#define VEC_DATATYPE(v) TYPE_OF((v)->data[0])

///
/// Validate whether a given `Vec` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `Vec` is not valid.
///
/// i[in] : Pointer to `Vec` object to validate.
///
/// SUCCESS : Continue execution, meaning given `Vec` object is most probably a valid `Vec`.
/// FAILURE : `abort`
///
#define ValidateVec(v)                                                                                                 \
    do {                                                                                                               \
        if (!(v)->alignment || (v)->length > (v)->capacity) {                                                          \
            LOG_FATAL("Invalid vec object.");                                                                          \
        }                                                                                                              \
        if ((v)->data) {                                                                                               \
            (void)(*(char *)(void *)((v)->data));                                                                      \
        }                                                                                                              \
    } while (0)


#endif // MISRA_STD_CONTAINER_VEC_TYPE_H
