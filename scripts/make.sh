#!/bin/sh
# Own build scripts
cd ../build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SIMD_AVX512=ON .
make