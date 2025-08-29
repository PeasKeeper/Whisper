#!/bin/bash
set -e

BUILD_SERVER=OFF
BUILD_CLIENT=OFF

if [ $# -eq 0 ]; then
  BUILD_SERVER=ON
  BUILD_CLIENT=ON
else
  for arg in "$@"; do
    case "$arg" in
      server)
        BUILD_SERVER=ON
        ;;
      client)
        BUILD_CLIENT=ON
        ;;
      *)
        echo "Unknown argument: $arg"
        echo "Usage: $0 [client] [server]"
        exit 1
        ;;
    esac
  done
fi

[ -d "build" ] && rm -rf build

cmake -S . -B build -DBUILD_SERVER=$BUILD_SERVER -DBUILD_CLIENT=$BUILD_CLIENT
make -C build
