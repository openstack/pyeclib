#!/bin/bash

# Copyright (c) 2013, Kevin Greenan (kmgreen2@gmail.com)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY
# THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
FILE_DIR=./test_files
DECODED_DIR=./decoded_files
FRAGMENT_DIR=./test_fragments
FILES=`echo ${FILE_DIR}`
TOOLS_DIR=../tools

if [ ! -d ${DECODED_DIR} ]; then
  mkdir ${DECODED_DIR}
fi

if [ ! -d ${FRAGMENT_DIR} ]; then
  mkdir ${FRAGMENT_DIR}
fi

TYPES="
jerasure_rs_vand
jerasure_rs_cauchy
flat_xor_hd_3
flat_xor_hd_4
isa_l_rs_vand
liberasurecode_rs_vand
"
NUM_DATAS="10 11 12"
RS_NUM_PARITIES="2 3 4"
XOR_NUM_PARITIES="6"

# Weird stuff happens when we try to rebuild and
# are beyond the FT of the code...  Make it fail
# gracefully

for TYPE in ${TYPES}; do
  for NUM_DATA in ${NUM_DATAS}; do
    rm ${DECODED_DIR}/*
    rm ${FRAGMENT_DIR}/*
    NUM_PARITIES=${RS_NUM_PARITIES}
    if [[ ${TYPE} == "flat_xor_hd"* ]]; then
      NUM_PARITIES=${XOR_NUM_PARITIES}
    fi
    for NUM_PARITY in ${NUM_PARITIES}; do
      let NUM_TOTAL=$(( NUM_DATA + NUM_PARITY))
      FAULT_TOL=${NUM_PARITY}
      if [[ ${TYPE} == "flat_xor_hd"* ]]; then
        FAULT_TOL="2"
      fi 
      for file in `cd ${FILES}; echo *; cd ..`; do
        python ${TOOLS_DIR}/pyeclib_encode.py ${NUM_DATA} ${NUM_PARITY} ${TYPE} ${FILE_DIR} ${file} ${FRAGMENT_DIR}
      done

      for file in `cd ${FILES}; echo *; cd ..`; do
        fragments=( `echo ${FRAGMENT_DIR}/${file}.*` )
        let i=0
        while (( $i < ${FAULT_TOL} )); do
          index=$(( RANDOM % NUM_TOTAL ))
          fragments[${index}]="" 
          let i=$i+1
        done
        python ${TOOLS_DIR}/pyeclib_decode.py ${NUM_DATA} ${NUM_PARITY} ${TYPE} ${fragments[*]} ${DECODED_DIR}/${file} 
        diff ${FILE_DIR}/${file} ${DECODED_DIR}/${file}.decoded
        if [[ $? != 0 ]]; then
          echo "${FILE_DIR}/${file} != ${DECODED_DIR}/${file}.decoded"
          exit 1
        fi
      done
    done
  done
done

rm ${DECODED_DIR}/*
rm ${FRAGMENT_DIR}/*
