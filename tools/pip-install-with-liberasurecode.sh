#!/bin/bash -xe
if [ -z "$VIRTUAL_ENV" ]; then
    echo "Expected VIRTUAL_ENV to be set!"
    exit 1
fi
if [ -z "$LIBERASURECODE_DIR" ]; then
    echo "Expected LIBERASURECODE_DIR to be set!"
    exit 1
fi
if [ ! -d "$LIBERASURECODE_DIR" ]; then
    git clone git://git.openstack.org/openstack/liberasurecode "$LIBERASURECODE_DIR"
fi
pushd "$LIBERASURECODE_DIR"
if [ -n "$LIBERASURECODE_REF" ]; then
    git fetch origin "$LIBERASURECODE_REF"
    git checkout FETCH_HEAD
fi
./autogen.sh
./configure --prefix "$VIRTUAL_ENV"
make
make install
popd
pip install "$@"
