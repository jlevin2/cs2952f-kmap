#!/bin/sh
export SERVICE_NAME=1
#gdb --args env
#--log-level debug
#LD_PRELOAD=/code/envoy.so /usr/local/bin/envoy -c /etc/service-envoy.yaml --service-cluster service${SERVICE_NAME} --log-level debug > /dev/null 2>&1  &
#LD_PRELOAD=/code/service.so python3 /code/service.py &
/usr/local/bin/envoy -c /etc/tinyserver-envoy.yaml --service-cluster service${SERVICE_NAME} --log-level error &
gcc -g -o tiny tiny.c
./tiny 8080

