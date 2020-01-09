#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_quotefr.sh;
sh $BASEDIR/run_quotefr.sh;

