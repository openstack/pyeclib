FILE_DIR=./test_files
DECODED_DIR=./decoded_files
FRAGMENT_DIR=./test_fragments
FILES=`echo ${FILE_DIR}`

if [ ! -d ${DECODED_DIR} ]; then
  mkdir ${DECODED_DIR}
fi

if [ ! -d ${FRAGMENT_DIR} ]; then
  mkdir ${FRAGMENT_DIR}
fi

TYPES="flat_xor_4 flat_xor_3 rs_vand rs_cauchy_orig"
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
    if [[ `echo flat_xor_4 flat_xor_3 | grep ${TYPE}` ]]; then
      NUM_PARITIES=${XOR_NUM_PARITIES}
    fi 
    for NUM_PARITY in ${NUM_PARITIES}; do
      let NUM_TOTAL=$(( NUM_DATA + NUM_PARITY))
      FAULT_TOL=${NUM_PARITY}
      if [[ ${TYPE} == "flat_xor_4" ]]; then
        FAULT_TOL="3"
      fi 
      if [[ ${TYPE} == "flat_xor_3" ]]; then
        FAULT_TOL="2"
      fi 
			for file in `cd ${FILES}; echo *; cd ..`; do
			  python ec_pyeclib_encode.py ${NUM_DATA} ${NUM_PARITY} ${TYPE} ${FILE_DIR} ${file} ${FRAGMENT_DIR}
			done
			
			for file in `cd ${FILES}; echo *; cd ..`; do
			  fragments=( `echo ${FRAGMENT_DIR}/${file}.*` )
			  let i=0
			  while (( $i < ${FAULT_TOL} )); do
			    index=$(( RANDOM % NUM_TOTAL ))
			    fragments[${index}]="" 
			    let i=$i+1
			  done
			  python ec_pyeclib_decode.py ${NUM_DATA} ${NUM_PARITY} ${TYPE} ${fragments[*]} ${DECODED_DIR}/${file} 
			  diff ${FILE_DIR}/${file} ${DECODED_DIR}/${file}.decoded
			  if [[ $? != 0 ]]; then
			    echo "${FILE_DIR}/${file} != ${DECODED_DIR}/${file}.decoded"
			    exit 1
			  fi
			done
		done
	done
done

