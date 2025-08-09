#!/bin/bash

# Сборка релизной версии
VER_INFO=""

if [[ "$#" -eq "1" ]]; then
  VER_INFO="-DAPPLICATION_VERSION=$1"
fi

pushd ..

rm -rfd build-lin-release
cmake -B build-lin-release -DCMAKE_BUILD_TYPE=Release ${VER_INFO}
cmake --build build-lin-release

popd

