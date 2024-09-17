# manylinux2010 has oldest build chain that can still build modern ISA-L
# 2021-02-06-3d322a5 is newest tag that still had 2.7 support
# aarch64 images never had 2.7, but 2021-05-01-a10bece is the newest that still had 3.5

ARG TARGET="x86_64"

FROM quay.io/pypa/manylinux2010_x86_64:2021-02-06-3d322a5 AS x86_64
FROM quay.io/pypa/manylinux2014_aarch64:2021-05-01-a10bece AS aarch64
FROM quay.io/pypa/musllinux_1_1_x86_64:2021-09-18-f12faf3 AS musl_x86_64
FROM quay.io/pypa/musllinux_1_1_aarch64:2021-09-18-f12faf3 AS musl_aarch64
FROM ${TARGET}

MAINTAINER OpenStack Swift

# can also take branch names, e.g. "master"
ARG LIBERASURECODE_TAG=1.6.4
ARG ISAL_TAG=v2.31.0

ARG SO_SUFFIX=-pyeclib
ENV SO_SUFFIX=${SO_SUFFIX}
ENV UID=1000
# Alternatively, try cp27-cp27m, cp27-cp27mu
ENV PYTHON_VERSION=cp35-cp35m

RUN mkdir /opt/src /output

# Fix up mirrorlist issues
RUN rm -f /etc/yum.repos.d/CentOS-SCLo-scl-rh.repo
RUN if [ -e /etc/yum.repos.d ]; then \
    sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo && \
    sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo && \
    sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/*.repo ; \
  fi

RUN if [ -n "$(type -p yum)" ]; then yum install -y zlib-devel ; fi
# Update auditwheel so it can improve our tag to manylinux1 automatically
# Not *too far*, though, since we've got the old base image
RUN /opt/_internal/tools/bin/pip install -U 'auditwheel<5.2'

# Server includes `Content-Encoding: x-gzip`, so ADD unwraps it
ADD https://www.nasm.us/pub/nasm/releasebuilds/2.16.01/nasm-2.16.01.tar.gz /opt/src/nasm.tar
RUN tar -C /opt/src -x -f /opt/src/nasm.tar
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
    CFLAGS="-DLIBERASURECODE_SO_SUFFIX='"'"'"${SO_SUFFIX}"'"'"'" ./configure --prefix=/usr && \
    make && \
    make install

COPY . /opt/src/pyeclib/
ENTRYPOINT ["/bin/sh", "-c", "/opt/python/${PYTHON_VERSION}/bin/python /opt/src/pyeclib/pack_wheel.py /opt/src/pyeclib/ --repair --so-suffix=${SO_SUFFIX} --wheel-dir=/output"]
