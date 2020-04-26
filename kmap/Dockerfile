# this has the latest dev version of envoy in /usr/local/bin
FROM envoyproxy/envoy-dev:latest

RUN apt update && apt install -y python3 bash gcc python3-pip gdb vim tmux htop net-tools curl gawk netcat lsof man iptables strace
RUN pip3 install -q Flask==0.11.1 requests==2.18.4
RUN mkdir /code


# Envoy admin page
EXPOSE 8081
EXPOSE 8080
EXPOSE 80

ENTRYPOINT ["/bin/bash"]