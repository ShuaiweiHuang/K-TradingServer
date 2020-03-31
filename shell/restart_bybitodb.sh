#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_bybitodb.sh;
sh $BASEDIR/run_bybitodb.sh;

