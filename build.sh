#!/bin/bash

mkdir -p build
pushd build
cmake ..
make
popd
server/build_python_stubs.sh
