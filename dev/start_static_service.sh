#!/bin/sh
export SERVICE_NAME=1
#gdb --args env
#--log-level debug
#LD_PRELOAD=/code/envoy.so /usr/local/bin/envoy -c /etc/service-envoy.yaml --service-cluster service${SERVICE_NAME} --log-level debug > /dev/null 2>&1  &
#LD_PRELOAD=/code/service.so python3 /code/service.py &
LD_PRELOAD=/code/envoy.so /usr/local/bin/envoy -c /code/tinyserver-envoy.yaml --service-cluster service${SERVICE_NAME} --log-level debug &
gcc -g -o tiny tiny.c
LD_PRELOAD=/code/service.so ./tiny 8080

