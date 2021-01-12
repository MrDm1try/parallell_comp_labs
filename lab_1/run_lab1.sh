#!/bin/bash

declare -a executables=("lab1-seq" "lab1-par-2" "lab1-par-3" "lab1-par-4" "lab1-par-5" "lab1-par-6")
declare -a N_set=("400" "1510" "2620" "3730" "4840" "5950" "7060" "8170" "9180" "10390" "11500")
FILENAME="N.csv"

> N.csv
echo -e ";" >> ${FILENAME}
echo -e ";" >> ${FILENAME}
echo -e ";" >> ${FILENAME}

for N in "${N_set[@]}"
do
    for exec in "${executables[@]}" 
    do
        X=$(./${exec} ${N})
        y=(${X//$'\n'/ })
        line_num=1
        for num in ${X}; do
            sed -e "${line_num}s/\$/$(sed 's/\./,/g' <<< ${num});/" -i ${FILENAME}
            ((line_num=line_num+1))
        done
    done
    sed -e "s/\$/;/" -i ${FILENAME}
done

exit 0
