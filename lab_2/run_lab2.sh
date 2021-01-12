#!/bin/bash
set -eu

declare -a N_set=("400" "1510" "2620" "3730" "4840" "5950" "7060" "8170" "9280" "10390" "11500")
declare -a M_set=("1" "2" "3" "4" "5" "6")
TIME_TEMPLATE="lab2_template.csv"
TIME_OUTPUT="lab2_output.csv"
X_TEMPLATE="lab2_x_template.csv"
X_OUTPUT="lab2_x_output.csv"
EXECUTABLE="lab2"

NUM_OF_TRIES=3

function add_to_line() {
  FILE_NAME="${1}"
  LINE_NUM="${2}"
  TEXT="${3}"
  sed -e "${LINE_NUM}s/\$/${TEXT};/" -i "${FILE_NAME}"
}

cp "./${TIME_TEMPLATE}" "./${TIME_OUTPUT}"
cp "./${X_TEMPLATE}" "./${X_OUTPUT}"

for M in "${M_set[@]}"
do
    x_line_num=1
    add_to_line ${X_OUTPUT} ${x_line_num} ${M}
    ((x_line_num=x_line_num+1))
    time_line_num=2
    for N in "${N_set[@]}"
    do
        X=$(./${EXECUTABLE} ${N} ${M})
        y=(${X//$'\n'/ })
        min_time=${y[1]}
        for (( meh=2; meh<=${NUM_OF_TRIES}; meh++ ))
        do
          X=$(./${EXECUTABLE} ${N} ${M})
          y=(${X//$'\n'/ })
          if [ "${y[1]}" -lt "${min_time}" ]
          then
            min_time=${y[1]}
          fi
        done
        add_to_line ${TIME_OUTPUT} ${time_line_num} ${min_time}
        add_to_line ${X_OUTPUT} ${x_line_num} $(sed 's/\./,/g' <<< ${y[2]})
        ((x_line_num=x_line_num+1))
        ((time_line_num=time_line_num+1))
    done
done

exit 0
