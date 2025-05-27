# MisraStdC

A library to make programming in C less painful for you and me.

Features :
- MSVC, GCC, Clang, all three major compilers supported
- Generic containers
  - `Vec(T)` : Work with any type in a type-safe manner with strict type checking.
  - `Str`    : Just a `typedef` of `Vec(char)` but provides it's own wrapper functions.
  - `Map(K, V)` : Generic key-value hash-map storage container (Work in progress...)
  - `Int` : A custom big int implementation (Work in progress...)
- Rust style Fmt IO
  - `WriteFmt`, `ReadFmt` : To write and read from standard I/O in a type-safe formatted manner.
  - `StrWriteFmt`, `StrReadFmt` :  To write and read from strigs in a type-safe formatted manner.

Take a look at [docs](https://docs.brightprogrammer.in) to get a list of functions and generic macros with their usage examples.

## Example

I'm busy with work and maintaining this library so no time to write good examples here. Consider
taking a look at the source code. I try to heavily document my code so the code itself acts as a
documentation.

Any help regarding improving documentation or adding new generic features is much appreciated.

## License

All files in this repo that are copyrighted by me are available under Apache 2.0 License if you're
using this for non-commercial usage and under GPLv3 if you're using this for any commercial use case.
I also reserve the right to make the licensing of copyrighted files less restrictive for any entity
I wish to do it for. This means if you're a commercial entity and if you have my explicit permission
you can use it under a license no more restrictive than GPLv3.

I intend to keep this library as open source and accessible as possible.

### Apache 2.0 (For Non-Commercial Use Case)

```
Copyright 2025 Siddharth Mishra

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

### GNU GPL 3.0 (For Commercial Use Case)

```
Copyright (C) 2025  Siddharth Mishra

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
