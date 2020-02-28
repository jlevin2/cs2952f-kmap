#!/bin/sh

python3 /code/service.py &
#gdb --args env
LD_PRELOAD=/code/library.so /usr/local/bin/envoy -c /etc/service-envoy.yaml --service-cluster service
