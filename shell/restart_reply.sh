#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_reply.sh;
sh $BASEDIR/run_reply.sh;

