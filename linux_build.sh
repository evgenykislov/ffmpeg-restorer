#!/bin/bash

rm -rfd build
cmake -B build
cmake --build build
sudo cmake --install build
