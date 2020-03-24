#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_ftxodb.sh;
sh $BASEDIR/run_ftxodb.sh;

