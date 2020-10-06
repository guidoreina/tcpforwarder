tcpforwarder
============
`tcpforwarder` accepts connections on one or more ports and forwards the received data to one or more upstream servers (the upstream servers don't send any data back).

When a new connection is received, `tcpforwarder` connects to all the defined upstream servers and forwards them the received data (simultaneous connections are allowed).

`tcpforwarder` can be started with more than thread. Each thread attaches to the same ports and uses epoll to handle the network events.


```
Usage: ./tcpforwarder [--bind <ip-port-range>]+ [--upstream-server <ip-port>]+ [--number-workers <number-workers>]
<ip-port-range> ::= <ip-port> | <ip-address>:<port-range>
<ip-port> ::= <ip-address>:<port>
<ip-address> ::= <ipv4-address> | <ipv6-address>
<port-range> ::= <port>-<port>

Minimum number of workers: 1.
Maximum number of workers: 32.
Default number of workers: 2.
```
