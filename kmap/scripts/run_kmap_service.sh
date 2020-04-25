#!/bin/sh
export SERVICE_NAME=1

rm /dev/shm/*

kill -9 `pgrep tiny`
kill -9 `pgrep envoy`


# run the binaries
LD_PRELOAD=/code/service.so /code/tiny 8080 &
LD_PRELOAD=/code/envoy.so /usr/local/bin/envoy -c /code/tinyserver-envoy.yaml --service-cluster service${SERVICE_NAME} --log-level warning &
