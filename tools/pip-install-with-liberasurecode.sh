#!/bin/bash -xe
if [ -z "$VIRTUAL_ENV" ]; then
    echo "Expected VIRTUAL_ENV to be set!"
    exit 1
fi

if [ -n "$ISAL_DIR" ]; then
    if [ ! -d "$ISAL_DIR" ]; then
        git clone git://github.com/01org/isa-l.git "$ISAL_DIR"
    fi
    pushd "$ISAL_DIR"
    ./autogen.sh
    ./configure --prefix "$VIRTUAL_ENV"
    make
    make install
    popd
fi

if [ -n "$GFCOMPLETE_DIR" ]; then
    if [ ! -d "$GFCOMPLETE_DIR" ]; then
        git clone https://github.com/ceph/gf-complete.git "$GFCOMPLETE_DIR"
    fi
    pushd "$GFCOMPLETE_DIR"
    ./autogen.sh
    ./configure --prefix "$VIRTUAL_ENV"
    make
    make install
    popd
fi

if [ -n "$JERASURE_DIR" ]; then
    if [ -z "$GFCOMPLETE_DIR" ]; then
        echo "JERASURE_DIR requires that GFCOMPLETE_DIR be set!"
        exit 1
    fi
    if [ ! -d "$JERASURE_DIR" ]; then
        git clone https://github.com/ceph/jerasure.git "$JERASURE_DIR"
    fi
    pushd "$JERASURE_DIR"
    autoreconf --force --install
    LD_LIBRARY_PATH="$VIRTUAL_ENV"/lib ./configure --prefix "$VIRTUAL_ENV" LDFLAGS="-L$VIRTUAL_ENV/lib" CPPFLAGS="-I$VIRTUAL_ENV/include"
    make
    make install
    popd
fi

if [ -z "$LIBERASURECODE_DIR" ]; then
    echo "Expected LIBERASURECODE_DIR to be set!"
    exit 1
fi
if [ ! -d "$LIBERASURECODE_DIR" ]; then
    git clone https://git.openstack.org/openstack/liberasurecode "$LIBERASURECODE_DIR"
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
