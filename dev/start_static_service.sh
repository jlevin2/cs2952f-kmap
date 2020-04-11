#!/bin/sh
export SERVICE_NAME=1
LD_PRELOAD=/code/service.so python3 /code/service.py &
#gdb --args env
#--log-level debug
LD_PRELOAD=/code/envoy.so /usr/local/bin/envoy -c /etc/service-envoy.yaml --service-cluster service${SERVICE_NAME} --log-level debug