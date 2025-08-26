# manylinux2010 has oldest build chain that can still build modern ISA-L

ARG TARGET="x86_64"

FROM quay.io/pypa/manylinux2010_x86_64:latest AS x86_64
FROM quay.io/pypa/manylinux2014_aarch64:latest AS aarch64
FROM quay.io/pypa/musllinux_1_1_x86_64:latest AS musl_x86_64
FROM quay.io/pypa/musllinux_1_1_aarch64:latest AS musl_aarch64
FROM ${TARGET}

LABEL org.opencontainers.image.authors="OpenStack Swift"

# can also take branch names, e.g. "master"
ARG LIBERASURECODE_TAG=1.7.1
ARG ISAL_TAG=v2.31.1

ARG SO_SUFFIX=-pyeclib
ENV SO_SUFFIX=${SO_SUFFIX}
ENV UID=1000
ENV PYTHON_VERSION=cp310-cp310

RUN mkdir /opt/src /output

# Fix up mirrorlist issues
RUN rm -f /etc/yum.repos.d/CentOS-SCLo-scl-rh.repo
RUN if [ -e /etc/yum.repos.d ]; then \
    sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo && \
    sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo && \
    sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/*.repo ; \
  fi

RUN if [ -n "$(type -p yum)" ]; then yum install -y zlib-devel ; fi
# Update setuptools for pyproject.toml support
RUN "/opt/python/${PYTHON_VERSION}/bin/python3" -m pip install -U setuptools packaging

ADD https://github.com/netwide-assembler/nasm/archive/refs/tags/nasm-2.15.05.tar.gz /opt/src/nasm.tar.gz
RUN tar -C /opt/src -xz -f /opt/src/nasm.tar.gz
RUN cd /opt/src/nasm-* && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make nasm && \
    install -c nasm /usr/bin/nasm

ADD https://github.com/intel/isa-l/archive/${ISAL_TAG}.tar.gz /opt/src/isa-l.tar.gz
RUN tar -C /opt/src -x -f /opt/src/isa-l.tar.gz -z
RUN cd /opt/src/isa-l-* && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && \
    make install

ADD https://github.com/openstack/liberasurecode/archive/${LIBERASURECODE_TAG}.tar.gz /opt/src/liberasurecode.tar.gz
RUN tar -C /opt/src -x -f /opt/src/liberasurecode.tar.gz -z
RUN cd /opt/src/liberasurecode*/ && \
    ./autogen.sh && \
    CFLAGS="-DLIBERASURECODE_SO_SUFFIX='"'"'"${SO_SUFFIX}"'"'"'" ./configure --prefix=/usr --disable-mmi && \
    make && \
    make install

COPY . /opt/src/pyeclib/
# Ensure licenses for liberasurecode and isa-l are included in the packed wheel
RUN cp /opt/src/liberasurecode*/COPYING /opt/src/pyeclib/LICENSE-liberasurecode && \
    cp /opt/src/isa-l-*/LICENSE /opt/src/pyeclib/LICENSE-isal
ENTRYPOINT ["/bin/sh", "-c", "/opt/python/${PYTHON_VERSION}/bin/python3 /opt/src/pyeclib/pack_wheel.py /opt/src/pyeclib/ --repair --so-suffix=${SO_SUFFIX} --wheel-dir=/output --require-isal"]
