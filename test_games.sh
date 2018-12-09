RESULT_FOLDER=tests

PNAME=SilverBullet

GAMES=10
CONT=0

for (( i = 0; i < $GAMES; i++ )); do
    SEED=$(shuf -i 1-100000 -n 1)
    RESULT=$(./Game $PNAME Dummy Dummy Dummy -s $SEED -i default.cnf -o ${RESULT_FOLDER}/$SEED.res 2>&1 >/dev/null | tee ${RESULT_FOLDER}/$SEED | tail -n 2 | head -n 1 | cut -d' ' -f 3)
    echo $RESULT
    if [ "$RESULT" = $PNAME ]; then
        mv ${RESULT_FOLDER}/$SEED.res ${RESULT_FOLDER}/W_$SEED.res
        mv ${RESULT_FOLDER}/$SEED ${RESULT_FOLDER}/W_$SEED.txt
        CONT=$((CONT+1))
    else
        mv ${RESULT_FOLDER}/$SEED.res ${RESULT_FOLDER}/L_$SEED.res
        mv ${RESULT_FOLDER}/$SEED ${RESULT_FOLDER}/L_$SEED.txt
    fi
done

echo $CONT/$GAMES
