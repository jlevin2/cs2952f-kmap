# cs2952f-kmap
Optimizing Envoy networking by using LD_PRELOAD to use shared memory instead of the local network stack.

## Research + Resources
https://drive.google.com/file/d/1n-h235tm8DnL_RqxTTA95rgGtrLkBsyr/view

Probably will want to use init containers to run the preload:
https://kubernetes.io/docs/concepts/workloads/pods/init-containers/

Envoy source:
https://github.com/envoyproxy/envoy

Build/test:
https://github.com/envoyproxy/envoy/tree/master/ci
