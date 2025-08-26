#!/bin/bash
set -e
[ -d "build" ] && rm -rf build
cmake -S . -B build -DBUILD_SERVER=ON -DBUILD_CLIENT=ON
make -C build
