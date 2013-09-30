NUM_DATA=10
NUM_PARITY=5
let NUM_TOTAL=$(( NUM_DATA + NUM_PARITY))
TYPE="flat_xor_4"
FAULT_TOL=3
FILE_DIR=./test_files
DECODED_DIR=./decoded_files
FRAGMENT_DIR=./test_fragments
FILES=`echo ${FILE_DIR}`

rm ${DECODED_DIR}/*
rm ${FRAGMENT_DIR}/*

if [ ! -d ${DECODED_DIR} ]; then
  mkdir ${DECODED_DIR}
fi

if [ ! -d ${FRAGMENT_DIR} ]; then
  mkdir ${FRAGMENT_DIR}
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
done
