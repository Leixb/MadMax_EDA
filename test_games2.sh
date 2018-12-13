#!/usr/bin/env bash

GAMES=${1:-10}
RESULT_FOLDER=${2:-tests}
PL1=${3:-Dummy}
PL2=${4:-Dummy}
PL3=${5:-Dummy}
PL4=${6:-Dummy}

mkdir -p $RESULT_FOLDER
CONT=0

POINTS=0

echo "-------------------------------------------------------------------------------"
echo Player: $PL1 vs $PL2 $PL3 $PL4
echo Folder: $RESULT_FOLDER
echo Games:  $GAMES
echo "-------------------------------------------------------------------------------"

for (( i = 1; i <= $GAMES; i++ )); do
    SEED=$(shuf -i 0-2147483647 -n 1)
    printf -v SEED "%010d" $SEED
    OUT_FILE="${RESULT_FOLDER}/${SEED}"

    echo -ne "Game $i/$GAMES\tSEED: $SEED RUNNING ...\r"

    ./Game $PL1 $PL2 $PL3 $PL4 -s $SEED -i default.cnf -o "${OUT_FILE}.res" 2>"${OUT_FILE}"

    WINNER=$(tail -n 2 "${OUT_FILE}" | head -n 1 | cut -d' ' -f 3)

    POINTS_PNAME=$(tail -n 6 ${OUT_FILE}| grep "${PL1} got score" | cut -d' ' -f 6 | sort | head -n 1)
    POINTS_WINNER=$(tail -n 6 ${OUT_FILE}| grep "${WINNER} got score" | cut -d' ' -f 6 | sort | head -n 1)
    POINTS=$((POINTS+POINTS_PNAME))

    echo -e "Game $i/$GAMES\tSEED: $SEED WINNER: $WINNER ($POINTS_WINNER)"

    if [ "$WINNER" = "$PL1" ]; then
        mv "${OUT_FILE}.res" "${RESULT_FOLDER}/W_$SEED.res"
        mv "${OUT_FILE}" "${RESULT_FOLDER}/W_$SEED.txt"
        CONT=$((CONT+1))
    else
        mv "${OUT_FILE}.res" "${RESULT_FOLDER}/L_$SEED.res"
        mv "${OUT_FILE}" "${RESULT_FOLDER}/L_$SEED.txt"
        echo -e "\t\t\t\t\t $PL1 ($POINTS_PNAME)"
    fi
done

echo "-------------------------------------------------------------------------------"

echo "WON: $CONT/$GAMES ($(((CONT*100)/GAMES))%)"
echo "MEAN: $((POINTS/GAMES))"

echo "-------------------------------------------------------------------------------"

