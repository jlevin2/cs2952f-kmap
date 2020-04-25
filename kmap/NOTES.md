1. Run `sudo docker build -t envoy .`
2. Run `sudo docker run -itd --privileged -P --name kmap envoy`
- Use -P flag to randomly map exposed ports
3. Run `sudo docker attach kmap`, which will bring up a shell for you.

If you want to mount the volume to compile/work
docker run -itd --privileged -P -v /Users/JoshLevin/Desktop/college/current/cs2952f/cs2952f-kmap/kmap:/code --name kmap envoy

The outer world will send the TCP traffic to port 80, which envoy will then handle accordingly

Update:
Running front-proxy example (front-edge envoy, unadjusted) with services as backends
https://www.envoyproxy.io/docs/envoy/latest/start/sandboxes/front_proxy.html
Adjust Dockerfile-service for building service containers
Use start_service.sh script to load the shared library
1. `docker-compose pull`
2. `docker-compose up --build -d`
3. `docker-compose ps`

To enter a container (shell)
e.g service1, service2
`docker-compose exec <container_name> /bin/bash`

Envoy wraps syscalls in source/common/api/posix
All networking is in net

Imports "echo '#include <unistd.h>' | gcc -E -x c - > unistd.preprocessed"
Read unistd.preprocessed to see imported h
unistd.h lives in `/usr/include`

GLIBC Docs:
https://sourceware.org/glibc/wiki/HomePage


Super great resource: https://tbrindus.ca/correct-ld-preload-hooking-libc/

