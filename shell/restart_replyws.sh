#! /bin/sh
BASEDIR=$(dirname $0)
echo $BASEDIR
sh $BASEDIR/kill_replyws.sh;
sh $BASEDIR/run_replyws.sh;
sh $BASEDIR/kill_quote.sh;
sh $BASEDIR/run_quote.sh;

