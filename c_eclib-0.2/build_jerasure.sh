#!/bin/sh

CURR_DIR=${PWD}
TMP_BUILD_DIR=${CURR_DIR}/tmp_build

OS_NAME=`uname`
SUPPORTED_OS=`echo "Darwin Linux" | grep ${OS_NAME}`

if [ -z ${SUPPORTED_OS} ]; then
  echo "${OS_NAME} is not supported!!!"
  exit 2
fi

WGET_PROG=`which wget`
CURL_PROG=`which curl`

if [[ -z ${WGET_PROG} && -z ${CURL_PROG} ]]; then
  echo "Please install wget or curl!!!"
  exit 2
fi

mkdir ${TMP_BUILD_DIR} || exit 3

GF_COMPLETE_SOURCE="http://www.kaymgee.com/Kevin_Greenan/Software_files/gf-complete.tar.gz"
JERASURE_SOURCE="http://www.kaymgee.com/Kevin_Greenan/Software_files/jerasure.tar.gz"

pushd ${TMP_BUILD_DIR}

if [ -n ${WGET_PROG} ]; then
  ${WGET_PROG} ${GF_COMPLETE_SOURCE}
  ${WGET_PROG} ${JERASURE_SOURCE}
else
  ${CURL_PROG} -O ${GF_COMPLETE_SOURCE}
  ${CURL_PROG} -O ${JERASURE_SOURCE}
fi

# Build JErasure and GF-Complete
LIB_DEPS="gf-complete jerasure"

for lib in ${LIB_DEPS}; do
  tar xf ${lib}.tar.gz
  pushd ${lib}
    chmod 0755 configure
    ./configure
    make
    [ $? -ne 0 ] && popd && exit 4
  popd
done

popd

#GF_COMPLETE_LIBS=${TMP_BUILD_DIR}/gf-complete/src/.libs
#GF_COMPLETE_INCL=${TMP_BUILD_DIR}/gf-complete/include
#
#if [[ ${OS_NAME} == "Linux" ]]; then
#  export LD_LIBRARY_PATH=${GF_COMPLETE_LIBS}
#else
#  export DYLD_LIBRARY_PATH=${GF_COMPLETE_LIBS}
#fi
#
#./configure LDFLAGS=-L$GFP/src/.libs/ CPPFLAGS=-I$GFP/include
