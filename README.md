# MisraStdC

A library to make programming in C less painful for you and me.

Features :
- MSVC, GCC, Clang, all three major compilers supported
- Generic containers
  - `Vec(T)` : Work with any type in a type-safe manner with strict type checking.
  - `Str`    : Just a `typedef` of `Vec(char)` but provides it's own wrapper functions.
  - `Map(K, V)` : Work in progress...
- Rust style Fmt IO
  - `WriteFmt`, `ReadFmt` : To write and read from standard I/O in a type-safe formatted manner.
  - `StrWriteFmt`, `StrReadFmt` :  To write and read from strigs in a type-safe formatted manner.

Take a look at [docs](https://docs.brightprogrammer.in) to get a list of functions and generic macros with their usage examples.

## Example

I'm busy with work and maintaining this library so no time to write good examples here. Consider
taking a look at the source code. I try to heavily document my code so the code itself acts as a
documentation.

Any help regarding improving documentation or adding new generic features is much appreciated.
