#!/bin/sh
export SERVICE_NAME=1
python3 /code/service.py &
#gdb --args env
LD_PRELOAD=/code/library.so /usr/local/bin/envoy -c /etc/service-envoy.yaml --service-cluster service${SERVICE_NAME} --log-level debug
