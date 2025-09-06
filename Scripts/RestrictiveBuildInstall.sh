#!/bin/bash

# This script is designed to do a quick dirty build and install in a restrictive
# environment that does not have tools like meson and ninja for development.
# 
# This means the script is best fit for when you just need to build and use it!
# meson and ninja is still recommended with a good lsp for development environment
# but in some remote sever, where you only have permission to copy over files from
# your machine, you can only build it without these development tools and build environment.

# Setup installation environment
mkdir -pv $HOME/.local/include
mkdir -pv $HOME/.local/lib

# Install headers first
cp -r Include/* $HOME/.local/include/

# A different directory to store all build files
mkdir -pv Build
cd Build

# Build, archive, install, clean
gcc -lm -std=c11 -I$HOME/.local/include -c $(find ../Source -type f -name '*.c')
ar rcs libmisra_std.a *.o
cp libmisra_std.a $HOME/.local/lib
rm *.o

# Then compile and use programs like
# Order of arguments matters somehow here. Here I'm compiling sol.c
# gcc -std=c11 -lm -I$HOME/.local/include sol.c -L$HOME/.local/lib -lmisra_std -o sol
