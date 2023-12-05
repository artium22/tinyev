#!/usr/bin/env bash

# Setup
set -e

dir=
if [ $# -eq 0 ]; then
    echo "Use default dir"
    dir=builddir
else
    dir="$1"
fi

meson setup $dir
ninja -C $dir