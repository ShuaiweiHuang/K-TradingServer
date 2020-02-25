#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_ftxq.sh;
sh $BASEDIR/run_ftxq.sh;

