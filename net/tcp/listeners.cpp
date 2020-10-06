#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "net/tcp/listeners.h"

net::tcp::listeners::~listeners()
{
  if (_M_fds) {
    for (size_t i = _M_used; i > 0; i--) {
      close(_M_fds[i - 1]);
    }

    free(_M_fds);
  }
}

bool net::tcp::listeners::listen(const char* address)
{
  socket::address addr;
  return ((addr.build(address)) && (listen(addr)));
}

bool net::tcp::listeners::listen(const char* address, in_port_t port)
{
  socket::address addr;
  return ((addr.build(address, port)) && (listen(addr)));
}

bool net::tcp::listeners::listen(const char* address,
                                 in_port_t minport,
                                 in_port_t maxport)
{
  // If the port range is valid...
  if (minport <= maxport) {
    // IPv4 address?
    {
      struct sockaddr_in sin;
      if (inet_pton(AF_INET, address, &sin.sin_addr) == 1) {
        sin.sin_family = AF_INET;

        memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

        // For each port in the range...
        for (unsigned port = minport; port <= maxport; port++) {
          sin.sin_port = htons(static_cast<in_port_t>(port));

          // Listen.
          if (!listen(*reinterpret_cast<const struct sockaddr*>(&sin),
                      sizeof(struct sockaddr_in))) {
            return false;
          }
        }

        return true;
      }
    }

    // IPv6 address?
    {
      struct sockaddr_in6 sin;
      if (inet_pton(AF_INET6, address, &sin.sin6_addr) == 1) {
        sin.sin6_family = AF_INET6;

        sin.sin6_flowinfo = 0;
        sin.sin6_scope_id = 0;

        // For each port in the range...
        for (unsigned port = minport; port <= maxport; port++) {
          sin.sin6_port = htons(static_cast<in_port_t>(port));

          // Listen.
          if (!listen(*reinterpret_cast<const struct sockaddr*>(&sin),
                      sizeof(struct sockaddr_in6))) {
            return false;
          }
        }

        return true;
      }
    }
  }

  return false;
}

bool net::tcp::listeners::listen(const struct sockaddr& addr, socklen_t addrlen)
{
  if (allocate()) {
    // Create socket.
    const int fd = ::socket(addr.sa_family, SOCK_STREAM | SOCK_NONBLOCK, 0);

    // If the socket could be created...
    if (fd != -1) {
      // Reuse address and port, bind and listen.
      const int optval = 1;
      if ((setsockopt(fd,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &optval,
                      sizeof(int)) == 0) &&
          (setsockopt(fd,
                      SOL_SOCKET,
                      SO_REUSEPORT,
                      &optval,
                      sizeof(int)) == 0) &&
          (bind(fd, &addr, addrlen) == 0) &&
          (::listen(fd, SOMAXCONN) == 0)) {
        _M_fds[_M_used++] = fd;

        return true;
      }

      close(fd);
    }
  }

  return false;
}

bool net::tcp::listeners::allocate()
{
  if (_M_used < _M_size) {
    return true;
  } else {
    const size_t size = (_M_size > 0) ? _M_size * 2 : allocation;

    int* fds = static_cast<int*>(realloc(_M_fds, size * sizeof(int)));

    if (fds) {
      _M_fds = fds;
      _M_size = size;

      return true;
    } else {
      return false;
    }
  }
}
