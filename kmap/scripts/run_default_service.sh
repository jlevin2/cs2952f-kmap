#!/bin/sh
export SERVICE_NAME=1

rm /dev/shm/*

kill -9 `pgrep tiny`
kill -9 `pgrep envoy`


# run the binaries
/code/tiny 8080 &
/usr/local/bin/envoy -c /code/tinyserver-envoy.yaml --service-cluster service${SERVICE_NAME} --log-level warning &
