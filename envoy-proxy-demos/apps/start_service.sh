#!/bin/sh
python3 /code/service.py &

#export LD_PRELOAD=/code/library.so
#envoy -c /etc/service-envoy.yaml --service-cluster service${SERVICE_NAME}
