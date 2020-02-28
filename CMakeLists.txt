cmake_minimum_required(VERSION 3.15)
project(cs2952f_kmap)

set(CMAKE_CXX_STANDARD 14)

add_executable(cs2952f_kmap
        envoy-proxy-demos/apps/library.c
        envoy-proxy-demos/assets/demo-app-arch.png
        envoy-proxy-demos/assets/demo-circuit-breaker.png
        envoy-proxy-demos/assets/demo-fault-injection.png
        envoy-proxy-demos/assets/demo-httproute-blue-green.png
        envoy-proxy-demos/assets/demo-httproute-header-match.png
        envoy-proxy-demos/assets/demo-httproute-simple-match.png
        envoy-proxy-demos/assets/demo-jaeger-tracing-req-trace.png
        envoy-proxy-demos/assets/demo-jaeger-tracing.png
        envoy-proxy-demos/assets/demo-timeouts-retries.png
        envoy-proxy-demos/assets/demo-zipkin-tracing-req-trace.png
        envoy-proxy-demos/assets/demo-zipkin-tracing.png
        envoy-proxy-demos/assets/jaeger-ui-servide-blue.png
        envoy-proxy-demos/assets/jaeger-ui-servide-green.png
        envoy-proxy-demos/assets/jaeger-ui-servide-red.png
        envoy-proxy-demos/assets/jaeger-ui.png
        envoy-proxy-demos/assets/zipkin-ui-servide-blue.png
        envoy-proxy-demos/assets/zipkin-ui-servide-green.png
        envoy-proxy-demos/assets/zipkin-ui.png
        envoy-proxy-demos/front-proxy/README.md
        envoy-proxy-demos/helpers/docker-build-up.sh
        envoy-proxy-demos/helpers/docker-down-remove.sh
        envoy-proxy-demos/helpers/docker-down.sh
        envoy-proxy-demos/helpers/envoyproxy-clusters.sh
        envoy-proxy-demos/helpers/envoyproxy-config_dump.sh
        envoy-proxy-demos/helpers/envoyproxy-reset_counters.sh
        envoy-proxy-demos/helpers/envoyproxy-runtime.sh
        envoy-proxy-demos/helpers/envoyproxy-stats.sh
        envoy-proxy-demos/helpers/parallel-requests.sh
        envoy-proxy-demos/helpers/send-requests.sh
        envoy-proxy-demos/httproute-simple-match/docker-compose.yml
        envoy-proxy-demos/httproute-simple-match/front-envoy.yaml
        envoy-proxy-demos/httproute-simple-match/README.md
        envoy-proxy-demos/httproute-simple-match/service-envoy.yaml
        envoy-proxy-demos/README.md
        dev/library.c
        dev/Dockerfile
        dev/NOTES.md
        dev/service.py
        dev/start_service.sh
        README.md)
