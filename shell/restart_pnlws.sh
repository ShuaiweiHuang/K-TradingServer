#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_pnlws.sh;
sh $BASEDIR/run_pnlws.sh;
