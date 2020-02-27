1. Run `sudo docker build -t envoy .`
2. Run `sudo docker run -itd --privileged --name <name> envoy`
3. Run `sudo docker attach <name>`, which will bring up a shell for you.