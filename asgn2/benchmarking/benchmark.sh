#!/usr/bin/env bash

if [ "$1" = "clean" ]; then
    rm powers
    exit 0
fi

#Goes ahead and rebuilds scripts on each RUNS
cc -o powers powers.c

#Just make, don't run benchmarks
if [ "$1" = "make" ]; then
    exit 0
fi

#Each command is run RUNS times
RUNS=10
COMMANDS[0]="./powers 10000000"
COMMANDS[1]="./powers 100000000"
COMMANDS[2]="./powers 1000000000"

#causes time to output only the real time the command took
TIMEFORMAT=%R

echo "Running ${#COMMANDS[@]} commands $RUNS times each"

#run all the commands in the background
CURCOMMAND=0
while [ $CURCOMMAND -lt ${#COMMANDS[@]} ]; do
    INDEX=0
    while [ $INDEX -lt $RUNS ]; do
        TMPI=$(( ($CURCOMMAND * $RUNS) + $INDEX))

        TMPFILES[$TMPI]=$(mktemp -t myBmk)

        (time ${COMMANDS[$CURCOMMAND]}) &> ${TMPFILES[$TMPI]} &

        INDEX=$(( $INDEX + 1 ))
    done

    CURCOMMAND=$(( $CURCOMMAND + 1 ))
done

#wait for all the commands to finish
wait

#add up the total latency of all commands
TOTALLAT=0
CURCOMMAND=0
while [ $CURCOMMAND -lt ${#COMMANDS[@]} ]; do
    INDEX=0
    while [ $INDEX -lt $RUNS ]; do
        TMPI=$(( ($CURCOMMAND * $RUNS) + $INDEX))

        TOTALLAT=$(echo "$TOTALLAT + $(cat ${TMPFILES[$TMPI]})" | bc)
        rm ${TMPFILES[$TMPI]}

        INDEX=$(( $INDEX + 1 ))
    done

    CURCOMMAND=$(( $CURCOMMAND + 1 ))
done

TOTALRUNS=$(( ${#COMMANDS[@]} * $RUNS ))
AVGLAT=$(echo "scale=2; $TOTALLAT / $TOTALRUNS" | bc)

echo "Average latency is $AVGLAT seconds"
