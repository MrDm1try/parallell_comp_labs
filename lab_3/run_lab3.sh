#!/bin/bash
set -eu

declare -a N_set=("400" "1510" "2620" "3730" "4840" "5950" "7060" "8170" "9280" "10390" "11500")
declare -a schedule_set=(static dynamic guided)
TEMPLATE="lab3_template.csv"
OUTPUT="lab3_output.csv"
N_LINE=1
TIME_LINE_NUM_START=2
X_LINE_NUM_START=46
EXECUTABLE_SEQ="lab3-seq"
EXECUTABLE_OMP="lab3"

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
    min_time=${y[1]}
    for (( meh=2; meh<=${NUM_OF_TRIES}; meh++ ))
    do
      X=$(./${EXECUTABLE} ${N})
      y=(${X//$'\n'/ })
      if [ "${y[1]}" -lt "${min_time}" ]
      then
        min_time=${y[1]}
      fi
    done
    add_to_line ${OUTPUT} ${time_line_num} ${min_time}
    add_to_line ${OUTPUT} ${x_line_num} $(sed 's/\./,/g' <<< ${y[2]})
}

echo -e ";;" > ${OUTPUT}
for (( meh=1; meh<=200; meh++ ))
do
    echo -e ";" >> ${OUTPUT}
done

for N in "${N_set[@]}"
do
    add_to_line ${OUTPUT} ${N_LINE} ${N}
    time_line_num=${TIME_LINE_NUM_START}
    x_line_num=${X_LINE_NUM_START}
    if [ "${N}" -eq "${N_set[0]}" ]
    then
        add_to_line ${OUTPUT} ${time_line_num} "seq"
    fi
    execute ${EXECUTABLE_SEQ} ${N} ${time_line_num} ${x_line_num}
    ((time_line_num=time_line_num+1))
    ((x_line_num=x_line_num+1))

    for schedule in "${schedule_set[@]}"
    do
    for chunk in 1 2 3 4 5 10 50 100 $((${N}/100)) $((${N}/20)) $((${N}/10)) $((${N}/5)) $((${N}/3)) $((${N}/2))
      do
        export OMP_SCHEDULE="${schedule},${chunk}"
        if [ "${N}" -eq "${N_set[0]}" ]
        then
            add_to_line ${OUTPUT} ${time_line_num} ${OMP_SCHEDULE}
        fi
        execute ${EXECUTABLE_OMP} ${N} ${time_line_num} ${x_line_num}
        ((time_line_num=time_line_num+1))
        ((x_line_num=x_line_num+1))
      done
    done
done

exit 0
