# this has the latest dev version of envoy in /usr/local/bin
FROM envoyproxy/envoy-dev:latest

# SETUP GCC 9
# RUN apt update
# RUN apt-get install build-essential software-properties-common -y
# RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
# RUN apt-get install gcc-9
# RUN apt-get install g++-9
# RUN rm /usr/bin/gcc
# RUN ln -s /usr/bin/gcc-9 /usr/bin/gcc
# RUN rm /usr/bin/g++
# RUN ln -s /usr/bin/g++-9 /usr/bin/g++

RUN apt update && apt install -y python3 bash gcc python3-pip gdb vim tmux htop net-tools curl gawk
RUN pip3 install -q Flask==0.11.1 requests==2.18.4
RUN mkdir /code
ADD service.py /code
ADD start_static_service.sh /usr/local/bin/start_static_service.sh
RUN chmod u+x /usr/local/bin/start_static_service.sh

ADD service-envoy.yaml /etc/service-envoy.yaml

ADD library.c /code
# ADD test.c /code
# ADD glibc /code/

#RUN gcc -shared -fPIC -g -o /code/library.so /code/library.c
RUN gcc -shared -fPIC /code/library.c  -o /code/library.so -ldl

# Envoy admin page
EXPOSE 8081
EXPOSE 8080
EXPOSE 80

ENTRYPOINT ["/bin/bash"]

#RUN /usr/local/bin/start_static_service.sh &