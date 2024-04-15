#!/bin/bash
if [ "$#" -ne 1 ]; then
    echo "用法：$0 <server/trigger>"
    app_name=server
    # exit 1
else
    app_name=$1
fi



pid=$(pgrep $app_name)

if [ -z "$pid" ]; then
    echo "server未运行"
else
    kill -15 $pid
fi