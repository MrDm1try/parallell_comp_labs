#!/bin/bash
set -eu

declare -a N_set=("3350" "4460" "5570" "6680" "7790" "8900" "10010" "11120" "12230" "13340" "14450" "15560" "16670" "17780" "18890" "20000" "21110" "22220" "23330" "24440" "25550" "26660" "27770" "28880" "29990" "31100" "32210" "33320" "34430" "35540" "36650")
TEMPLATE="lab5_template.csv"
OUTPUT="lab5_output.csv"
N_LINE=1
SEQ_TIME_LINE_NUM_START=2
OMP_TIME_LINE_NUM_START=4
PTHREADS_TIME_LINE_NUM_START=11
SEQ_X_LINE_NUM=18
OMP_X_LINE_NUM=19
PTHREADS_X_LINE_NUM=20
EXECUTABLE="lab5-extra"

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
    x_line_num="${4}"

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
      ((time_line_num=time_line_num+1))
      add_to_line ${OUTPUT} ${time_line_num} ${y[-8]}
      ((time_line_num=time_line_num+1))
      add_to_line ${OUTPUT} ${time_line_num} ${y[-7]}
      ((time_line_num=time_line_num+1))
      add_to_line ${OUTPUT} ${time_line_num} ${y[-6]}
      ((time_line_num=time_line_num+1))
      add_to_line ${OUTPUT} ${time_line_num} ${y[-5]}
      ((time_line_num=time_line_num+1))
      add_to_line ${OUTPUT} ${time_line_num} ${y[-4]}
    add_to_line ${OUTPUT} ${x_line_num} $(sed 's/\./,/g' <<< ${y[-1]})
}

cp "./${TEMPLATE}" "./${OUTPUT}"

for N in "${N_set[@]}"
do
    add_to_line ${OUTPUT} ${N_LINE} ${N}

    execute ${EXECUTABLE} ${N} ${PTHREADS_TIME_LINE_NUM_START} ${PTHREADS_X_LINE_NUM}
    echo ${N}
done

exit 0
