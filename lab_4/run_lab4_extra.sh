#!/bin/bash
set -eu

declare -a N_set=("3350" "4460" "5570" "6680" "7790" "8900" "10010" "11120" "12230" "13340" "14450" "15560" "16670" "17780" "18890" "20000" "21110" "22220" "23330" "24440" "25550" "26660" "27770" "28880" "29990" "31100" "32210" "33320" "34430" "35540" "36650")
TEMPLATE="lab4_template_extra.csv"
OUTPUT="lab4_output_extra.csv"
N_LINE=1
SEQ_TIME_LINE_NUM_START=2
PAR_TIME_LINE_NUM_START=16
SEQ_X_LINE_NUM=30
PAR_X_LINE_NUM=31
EXECUTABLE_SEQ="lab4-extra-seq"
EXECUTABLE_OMP="lab4-extra"

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

    for (( i=-11; i<=-2; i++ ))
    do
        add_to_line ${OUTPUT} ${TIME_LINE_NUM} ${y[i]}
        ((TIME_LINE_NUM=TIME_LINE_NUM+1))
    done
    add_to_line ${OUTPUT} ${X_LINE_NUM} $(sed 's/\./,/g' <<< ${y[-1]})
}

cp "./${TEMPLATE}" "./${OUTPUT}"

for N in "${N_set[@]}"
do
    add_to_line ${OUTPUT} ${N_LINE} ${N}
    execute ${EXECUTABLE_SEQ} ${N} ${SEQ_TIME_LINE_NUM_START} ${SEQ_X_LINE_NUM}
    execute ${EXECUTABLE_OMP} ${N} ${PAR_TIME_LINE_NUM_START} ${PAR_X_LINE_NUM}
done

exit 0
