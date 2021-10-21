# Lab 2



lab任务是写一个protocol stack **from bottom to top**.

底层给高层API.



#### Toolkit:

vnetUtils: 用一台主机模拟多台主机交互, 多个namespace实现context隔离.

mperf: 测试网络中两个节点传输的throughput和latency. 

Iperf(打流, 发流): 假设网络连接相对稳定, 但是robust不太好, mperf是iperf的self-edition.

Libpcap: c/c++ library for network traffic capture.

Wireshark: 看一下发的包是否能被Wireshark识别, 格式要正确.



valgrind: memory leakage.



传输层应用层之间的API必须conform, 其他层API可以改.

