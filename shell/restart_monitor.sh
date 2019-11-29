#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_monitor.sh;
sh $BASEDIR/run_monitor.sh;

