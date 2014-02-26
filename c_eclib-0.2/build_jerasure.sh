#!/bin/sh

# These directory stack functions are based upon the versions in the Korn
# Shell documentation - http://docstore.mik.ua/orelly/unix3/korn/ch04_07.htm.
dirs() {
  echo "$_DIRSTACK"
}
     
pushd() {
  dirname=$1
  cd ${dirname:?"missing directory name."} || return 1
  _DIRSTACK="$PWD $_DIRSTACK"
  echo "$_DIRSTACK"
}
		     
popd() {
  _DIRSTACK=${_DIRSTACK#* }
  top=${_DIRSTACK%% *}
  cd $top || return 1
  echo "$PWD"
}

download() {
  pkgurl="$1"

  WGET_PROG=`which wget`
  CURL_PROG=`which curl`

  if [ -z "${WGET_PROG}" ] && [ -z "${CURL_PROG}" ]; then
    echo "Please install wget or curl!!!"
    exit 2
  fi

  rm -f `basename ${pkgurl}`
  if [ -n ${WGET_PROG} ]; then
    ${WGET_PROG} ${pkgurl}
  else
    ${CURL_PROG} -O ${pkgurl}
  fi
}

# Checks
CURR_DIR=${PWD}
TMP_BUILD_DIR=${CURR_DIR}/tmp_build

OS_NAME=`uname`
SUPPORTED_OS=`echo "Darwin Linux" | grep ${OS_NAME}`

if [ -z "${SUPPORTED_OS}" ]; then
  echo "${OS_NAME} is not supported!!!"
  exit 2
fi

# Download sources for Jerasure and GF-complete
mkdir -p ${TMP_BUILD_DIR}
pushd ${TMP_BUILD_DIR}

gf_complete_SOURCE="http://www.kaymgee.com/Kevin_Greenan/Software_files/gf-complete.tar.gz"
Jerasure_SOURCE="http://www.kaymgee.com/Kevin_Greenan/Software_files/jerasure.tar.gz"

# Build JErasure and GF-Complete
LIB_DEPS="gf_complete Jerasure"
LDFLAGS=""
LIBS=""

for lib in ${LIB_DEPS}; do

  # Download and extract
  src="${lib}_SOURCE"
  url=$(eval echo \$${src})
  srcfile=`basename ${url}`

  download ${url}
  srcdir=$(tar tf ${srcfile} | sed -e 's,/.*,,' | uniq)

  # Extract and Build
  tar xf ${srcfile}
  pushd ${srcdir}
    chmod 0755 configure
    ./configure
    make
    [ $? -ne 0 ] && popd && popd && exit 4
  popd

  # Generate LDADD lines for c_eclib
  LIBDIR=$(find ${TMP_BUILD_DIR} -type f -name "lib${lib}.so.*" -printf '%h\n' | sort -u | head -1)
  LDFLAGS=" ${LDFLAGS} -L${LIBDIR} "
  LIBS=" ${LIBS} -l${lib} "
done

popd

echo "LDFLAGS=${LDFLAGS}"
echo ${LDFLAGS} > ${CURR_DIR}/.ldflags

echo "LIBS=${LIBS}"
echo ${LIBS} > ${CURR_DIR}/.libs
