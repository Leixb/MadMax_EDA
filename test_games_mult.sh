#!/usr/bin/env bash

GAMES=${1:-10}
RESULT_FOLDER=${2:-tests}
PNAME=${3:-SilverBullet}
P2=${4:-Dummy}
P3=${5:-Dummy}
P4=${6:-Dummy}

PVERSION=$(md5sum AI$PNAME.cc | cut -d' ' -f1)

mkdir -p $RESULT_FOLDER
mkdir -p $RESULT_FOLDER/bk
mv $RESULT_FOLDER/*.{txt,res} $RESULT_FOLDER/bk
CONT=0

for (( i = 0; i < $GAMES; i++ )); do
    SEED=$(shuf -i 0-2147483647 -n 1)
    printf -v PSEED "%010d" $SEED
    RESULT=$(./Game $PNAME $P2 $P3 $P4 -s $SEED -i default.cnf -o ${RESULT_FOLDER}/$PSEED.res 2>&1 >/dev/null | tee ${RESULT_FOLDER}/$PSEED | tail -n 2 | head -n 1 | cut -d' ' -f 3)
    echo "GAME $((i+1)) of $GAMES: SEED: $SEED. WINNER: $RESULT"
    if [ "$RESULT" = "$PNAME" ]; then
        mv "${RESULT_FOLDER}/$PSEED.res" "${RESULT_FOLDER}/W_$PSEED.res"
        mv "${RESULT_FOLDER}/$PSEED" "${RESULT_FOLDER}/W_$PSEED.txt"
        CONT=$((CONT+1))
    else
        mv "${RESULT_FOLDER}/$PSEED.res" "${RESULT_FOLDER}/L_$PSEED.res"
        mv "${RESULT_FOLDER}/$PSEED" "${RESULT_FOLDER}/L_$PSEED.txt"
    fi
done

echo "Won $CONT of $GAMES"
