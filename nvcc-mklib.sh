#! /usr/bin/env bash

if [ "$1" = --description ]; then
  echo "compile a .cpp file into a shared library"
  exit 0
fi

if [ "$1" = --options ]; then
  echo "--description --options"h
  exit 0
fi

source "$PLUMED_ROOT"/src/config/compile_options.sh

if [ $# != 2 ] || [[ "$1" != *.cpp && "$2" != *.cu ]]; then
  echo "ERROR"
  echo "type 'mklib file.cpp kernel.cu'"
  exit 1
fi

file="$1"
obj="${file%%.cpp}".o
kernel="${2%%.cu}"_kernel.o
lib="${file%%.cpp}".$soext

if [ ! -f "$file" ]; then
  echo "ERROR: I cannot find file $file"
  exit 1
fi

rm -f "$obj" "$lib"

nvcc "$2" -Xcompiler -fPIC -c -o "$kernel"

link_installed="nvcc -shared${link_installed#*-shared}"
link_installed=${link_installed/-rdynamic/-Xcompiler -rdynamic}
link_installed=${link_installed/-Wl,/-Xlinker }
link_installed=${link_installed/-fopenmp/-Xcompiler -fopenmp}
if test "$PLUMED_IS_INSTALLED" = yes; then
  eval "$compile" "$obj" "$file" && eval "$link_installed" "$lib" "$obj" "$kernel"
else
  eval "$compile" "$obj" "$file" && eval "$link_uninstalled" "$lib" "$obj" "$kernel"
fi
