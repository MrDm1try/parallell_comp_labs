#!/bin/bash
set -eu

TEMPLATE="lab5_template.csv"
OUTPUT="lab5_lower.csv"
N_LINE=1
SEQ_LINE_NUM_START=2
PTHREADS_LINE_NUM_START=4
EXECUTABLE_SEQ="lab4-seq-micro"
EXECUTABLE_PTHREADS="lab5-micro"

NUM_OF_TRIES=3

function add_to_line() {
  FILE_NAME="${1}"
  LINE_NUM="${2}"
  TEXT="${3}"
  sed -e "${LINE_NUM}s/\$/${TEXT};/" -i "${FILE_NAME}"
}

function execute() {
    EXECUTABLE="${1}"
    N="${2}"
    time_line_num="${3}"

    X=$(./${EXECUTABLE} ${N})
    y=(${X//$'\n'/ })
    min_time=${y[-2]}
    for (( meh=2; meh<=${NUM_OF_TRIES}; meh++ ))
    do
      X=$(./${EXECUTABLE} ${N})
      y=(${X//$'\n'/ })
      if [ "${y[-2]}" -lt "${min_time}" ]
      then
        min_time=${y[-2]}
      fi
    done
    add_to_line ${OUTPUT} ${time_line_num} ${min_time}
}

cp "./${TEMPLATE}" "./${OUTPUT}"

for (( N=${1}; N<=${2}; ((N=N+${3})) ))
do
    add_to_line ${OUTPUT} ${N_LINE} ${N}

    execute ${EXECUTABLE_SEQ} ${N} ${SEQ_LINE_NUM_START}
    execute ${EXECUTABLE_PTHREADS} ${N} ${PTHREADS_LINE_NUM_START}
done

exit 0
