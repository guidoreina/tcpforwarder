#include <stdlib.h>
#include <arpa/inet.h>
#include "net/socket/addresses.h"

net::socket::addresses::~addresses()
{
  if (_M_addresses) {
    free(_M_addresses);
  }
}

bool net::socket::addresses::add(const char* address)
{
  socket::address addr;
  return ((addr.build(address)) && (add(addr)));
}

bool net::socket::addresses::add(const char* address, in_port_t port)
{
  socket::address addr;
  return ((addr.build(address, port)) && (add(addr)));
}

bool net::socket::addresses::add(const char* address,
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

          // Add socket address.
          if (!add(*reinterpret_cast<const struct sockaddr*>(&sin),
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

          // Add socket address.
          if (!add(*reinterpret_cast<const struct sockaddr*>(&sin),
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

bool net::socket::addresses::add(const struct sockaddr& addr, socklen_t addrlen)
{
  // Allocate socket address (if needed).
  if (allocate()) {
    _M_addresses[_M_used++] = socket::address{addr, addrlen};
    return true;
  }

  return false;
}

bool net::socket::addresses::add(const socket::address& addr)
{
  // Allocate socket address (if needed).
  if (allocate()) {
    _M_addresses[_M_used++] = addr;
    return true;
  }

  return false;
}

bool net::socket::addresses::allocate()
{
  if (_M_used < _M_size) {
    return true;
  } else {
    const size_t size = (_M_size > 0) ? _M_size * 2 : allocation;

    socket::address*
      addrs = static_cast<socket::address*>(
                realloc(_M_addresses, size * sizeof(socket::address))
              );

    if (addrs) {
      _M_addresses = addrs;
      _M_size = size;

      return true;
    } else {
      return false;
    }
  }
}
