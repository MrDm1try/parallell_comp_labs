#!/bin/bash
set -eu

N1=1000
TEMPLATE="lab4_template.csv"
OUTPUT="lab4_lower.csv"
N_LINE=1
TIME_LINE_NUM_START=2
X_LINE_NUM_START=5
EXECUTABLE_SEQ="lab4-seq-micro"
EXECUTABLE_OMP="lab4-micro"

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
    TIME_LINE_NUM="${3}"
    X_LINE_NUM="${4}"

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
    add_to_line ${OUTPUT} ${x_line_num} $(sed 's/\./,/g' <<< ${y[-1]})
}

cp "./${TEMPLATE}" "./${OUTPUT}"

for (( N=${1}; N<=${2}; ((N=N+${3})) ))
do
    add_to_line ${OUTPUT} ${N_LINE} ${N}
    time_line_num=${TIME_LINE_NUM_START}
    x_line_num=${X_LINE_NUM_START}

    execute ${EXECUTABLE_SEQ} ${N} ${time_line_num} ${x_line_num}
    ((time_line_num=time_line_num+1))
    ((x_line_num=x_line_num+1))

    execute ${EXECUTABLE_OMP} ${N} ${time_line_num} ${x_line_num}
    ((time_line_num=time_line_num+1))
    ((x_line_num=x_line_num+1))
done

exit 0
