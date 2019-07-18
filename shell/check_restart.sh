#!/bin/bash

service=CVQuote
if (( $(ps -ef | grep -v grep | grep $service | wc -l) > 0 ))
then
echo "$service is running!!!"
else
cd /home/keanu.huang/CVTraderServer/shell/
./kill_quote.sh; ./run_quote.sh
fi
