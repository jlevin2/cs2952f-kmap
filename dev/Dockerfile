# this has the latest dev version of envoy in /usr/local/bin
FROM envoyproxy/envoy-dev:latest

RUN apt update && apt install -y python3 bash gcc python3-pip gdb vim tmux htop
RUN pip3 install -q Flask==0.11.1 requests==2.18.4
RUN mkdir /code
ADD service.py /code
ADD start_service.sh /usr/local/bin/start_service.sh
RUN chmod u+x /usr/local/bin/start_service.sh

ADD service-envoy.yaml /etc/service-envoy.yaml

ADD library.c /code
RUN gcc -shared -fPIC -g -o /code/library.so /code/library.c

ENTRYPOINT ["/bin/bash"]

#RUN /usr/local/bin/start_service.sh &