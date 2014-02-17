#!/bin/bash

CURR_DIR=${PWD}
TMP_BUILD_DIR=${CURR_DIR}/tmp_build

OS_NAME=`uname`
SUPPORTED_OS=`echo "Darwin Linux" | grep ${OS_NAME}`

if [[ -z ${SUPPORTED_OS} ]]; then
  echo "${OS_NAME} is not supported!!!"
  exit 2
fi

HAS_WGET=`which wget`
HAS_CURL=`which curl`

if [[ -z ${HAS_WGET} && -z ${HAS_CURL} ]]; then
  echo "Please install wget or curl!!!"
  exit 2
fi

mkdir ${TMP_BUILD_DIR}

cd ${TMP_BUILD_DIR}

if [[ -n ${HAS_WGET} ]]; then
  wget http://www.kaymgee.com/Kevin_Greenan/Software_files/gf-complete.tar.gz
else
  curl -O http://www.kaymgee.com/Kevin_Greenan/Software_files/gf-complete.tar.gz
fi

tar xvf gf-complete.tar.gz

if [[ -n ${HAS_WGET} ]]; then
  wget http://www.kaymgee.com/Kevin_Greenan/Software_files/jerasure.tar.gz
else
  curl -O http://www.kaymgee.com/Kevin_Greenan/Software_files/jerasure.tar.gz
fi

tar xvf jerasure.tar.gz

cd gf-complete && ./configure && make && make install

if [[ $? != 0 ]]; then
  cd ..
  exit 2
fi

cd ../jerasure && ./configure && make && make install

if [[ $? != 0 ]]; then
  cd ..
  exit 2
fi

cd ..

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
