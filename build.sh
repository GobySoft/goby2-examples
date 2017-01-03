#!/bin/bash

if [ -z "${GOBY2_EXAMPLES_CMAKE_FLAGS}" ]; then
    GOBY2_EXAMPLES_CMAKE_FLAGS=
fi

if [ -z "${GOBY2_EXAMPLES_MAKE_FLAGS}" ]; then
    GOBY2_EXAMPLES_MAKE_FLAGS=
fi

set -e -u
mkdir -p build

echo "Configuring..."
echo "cmake .. ${GOBY2_EXAMPLES_CMAKE_FLAGS}"
pushd build >& /dev/null
cmake .. ${GOBY2_EXAMPLES_CMAKE_FLAGS}
echo "Building..."
echo "make ${GOBY2_EXAMPLES_MAKE_FLAGS} $@"
make ${GOBY2_EXAMPLES_MAKE_FLAGS} $@
popd >& /dev/null
