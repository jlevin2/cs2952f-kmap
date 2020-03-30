1. Run `sudo docker build -t envoy .`
2. Run `sudo docker run -itd --privileged -P --name kmap envoy`
- Use -P flag to randomly map exposed ports
3. Run `sudo docker attach kmap`, which will bring up a shell for you.

If you want to mount the volume to compile/work
docker run -itd --privileged -P -v /Users/JoshLevin/Desktop/college/current/cs2952f/cs2952f-kmap/dev:/code --name kmap envoy

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

ENVOY Process Map:
7fd70f8e1000-7fd70faa1000 r-xp 00000000 08:01 262075                     /lib/x86_64-linux-gnu/libc-2.23.so
7fd70faa1000-7fd70fca1000 ---p 001c0000 08:01 262075                     /lib/x86_64-linux-gnu/libc-2.23.so
7fd70fca1000-7fd70fca5000 r--p 001c0000 08:01 262075                     /lib/x86_64-linux-gnu/libc-2.23.so
7fd70fca5000-7fd70fca7000 rw-p 001c4000 08:01 262075                     /lib/x86_64-linux-gnu/libc-2.23.so
7fd70fca7000-7fd70fcab000 rw-p 00000000 00:00 0 
7fd70fcab000-7fd70fcc3000 r-xp 00000000 08:01 262143                     /lib/x86_64-linux-gnu/libpthread-2.23.so
7fd70fcc3000-7fd70fec2000 ---p 00018000 08:01 262143                     /lib/x86_64-linux-gnu/libpthread-2.23.so
7fd70fec2000-7fd70fec3000 r--p 00017000 08:01 262143                     /lib/x86_64-linux-gnu/libpthread-2.23.so
7fd70fec3000-7fd70fec4000 rw-p 00018000 08:01 262143                     /lib/x86_64-linux-gnu/libpthread-2.23.so
7fd70fec4000-7fd70fec8000 rw-p 00000000 00:00 0 
7fd70fec8000-7fd70fecb000 r-xp 00000000 08:01 262088                     /lib/x86_64-linux-gnu/libdl-2.23.so
7fd70fecb000-7fd7100ca000 ---p 00003000 08:01 262088                     /lib/x86_64-linux-gnu/libdl-2.23.so
7fd7100ca000-7fd7100cb000 r--p 00002000 08:01 262088                     /lib/x86_64-linux-gnu/libdl-2.23.so
7fd7100cb000-7fd7100cc000 rw-p 00003000 08:01 262088                     /lib/x86_64-linux-gnu/libdl-2.23.so
7fd7100cc000-7fd7100d3000 r-xp 00000000 08:01 262149                     /lib/x86_64-linux-gnu/librt-2.23.so
7fd7100d3000-7fd7102d2000 ---p 00007000 08:01 262149                     /lib/x86_64-linux-gnu/librt-2.23.so
7fd7102d2000-7fd7102d3000 r--p 00006000 08:01 262149                     /lib/x86_64-linux-gnu/librt-2.23.so
7fd7102d3000-7fd7102d4000 rw-p 00007000 08:01 262149                     /lib/x86_64-linux-gnu/librt-2.23.so
7fd7102d4000-7fd7103dc000 r-xp 00000000 08:01 262107                     /lib/x86_64-linux-gnu/libm-2.23.so
7fd7103dc000-7fd7105db000 ---p 00108000 08:01 262107                     /lib/x86_64-linux-gnu/libm-2.23.so
7fd7105db000-7fd7105dc000 r--p 00107000 08:01 262107                     /lib/x86_64-linux-gnu/libm-2.23.so
7fd7105dc000-7fd7105dd000 rw-p 00108000 08:01 262107                     /lib/x86_64-linux-gnu/libm-2.23.so
7fd7105dd000-7fd7105de000 r-xp 00000000 08:01 917511                     /code/library.so
7fd7105de000-7fd7107dd000 ---p 00001000 08:01 917511                     /code/library.so
7fd7107dd000-7fd7107de000 r--p 00000000 08:01 917511                     /code/library.so
7fd7107de000-7fd7107df000 rw-p 00001000 08:01 917511                     /code/library.so
7fd7107df000-7fd710805000 r-xp 00000000 08:01 262055                     /lib/x86_64-linux-gnu/ld-2.23.so
7fd7109e1000-7fd7109e2000 ---p 00000000 00:00 0 
7fd7109e2000-7fd7109e6000 rw-p 00000000 00:00 0 
7fd7109e6000-7fd7109e7000 ---p 00000000 00:00 0 
7fd7109e7000-7fd7109fe000 rw-p 00000000 00:00 0 
7fd710a02000-7fd710a03000 rw-s 00000000 00:77 52613                      /dev/shm/envoy_shared_memory_0
7fd710a03000-7fd710a04000 rw-p 00000000 00:00 0 
7fd710a04000-7fd710a05000 r--p 00025000 08:01 262055                     /lib/x86_64-linux-gnu/ld-2.23.so
7fd710a05000-7fd710a06000 rw-p 00026000 08:01 262055                     /lib/x86_64-linux-gnu/ld-2.23.so
7fd710a06000-7fd710a07000 rw-p 00000000 00:00 0 

Shared libraries:
/lib/x86_64-linux-gnu/libc-2.23.so
/lib/x86_64-linux-gnu/libpthread-2.23.so
/lib/x86_64-linux-gnu/libdl-2.23.so
/lib/x86_64-linux-gnu/librt-2.23.so
/lib/x86_64-linux-gnu/libm-2.23.so

/lib/x86_64-linux-gnu/ld-2.23.so
/lib/x86_64-linux-gnu/ld-2.23.so

GLIBC Docs:
https://sourceware.org/glibc/wiki/HomePage


Super great resource: https://tbrindus.ca/correct-ld-preload-hooking-libc/


WORKED:: gcc -shared -fPIC library.c  -o library.so -ldl
