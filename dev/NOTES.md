1. Run `sudo docker build -t envoy .`
2. Run `sudo docker run -itd --privileged -p --name <name> envoy`
- Added -p in order to expose 8081 envoy port
3. Run `sudo docker attach <name>`, which will bring up a shell for you.

The outer world will send the TCP traffic to port 80, which envoy will then handle accordingly