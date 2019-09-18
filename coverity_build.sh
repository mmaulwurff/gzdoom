#!/bin/bash

cd build
cmake --build . -- --jobs=2 --keep-going
