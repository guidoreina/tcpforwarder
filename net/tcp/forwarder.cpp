#include "net/tcp/forwarder.h"

net::tcp::forwarder::forwarder(size_t nworkers)
{
  if (nworkers == 0) {
    _M_nworkers = 1;
  } else if (nworkers > max_workers) {
    _M_nworkers = max_workers;
  } else {
    _M_nworkers = nworkers;
  }
}

net::tcp::forwarder::~forwarder()
{
  // Stop threads (if running).
  stop();
}

bool net::tcp::forwarder::listen(const char* address)
{
  socket::address addr;
  return ((addr.build(address)) && (listen(addr)));
}

bool net::tcp::forwarder::listen(const char* address, in_port_t port)
{
  socket::address addr;
  return ((addr.build(address, port)) && (listen(addr)));
}

bool net::tcp::forwarder::listen(const char* address,
                                 in_port_t minport,
                                 in_port_t maxport)
{
  // For each worker thread...
  for (size_t i = 0; i < _M_nworkers; i++) {
    // Listen.
    if (!_M_workers[i].listen(address, minport, maxport)) {
      return false;
    }
  }

  return true;
}

bool net::tcp::forwarder::listen(const struct sockaddr& addr, socklen_t addrlen)
{
  // For each worker thread...
  for (size_t i = 0; i < _M_nworkers; i++) {
    // Listen.
    if (!_M_workers[i].listen(addr, addrlen)) {
      return false;
    }
  }

  return true;
}

bool net::tcp::forwarder::listen(const socket::address& addr)
{
  return listen(static_cast<const struct sockaddr&>(addr), addr.length());
}

bool net::tcp::forwarder::add_upstream_server(const char* address)
{
  return _M_upstream_addresses.add(address);
}

bool net::tcp::forwarder::add_upstream_server(const char* address,
                                              in_port_t port)
{
  return _M_upstream_addresses.add(address, port);
}

bool net::tcp::forwarder::add_upstream_server(const struct sockaddr& addr,
                                              socklen_t addrlen)
{
  return _M_upstream_addresses.add(addr, addrlen);
}

bool net::tcp::forwarder::add_upstream_server(const socket::address& addr)
{
  return _M_upstream_addresses.add(addr);
}

bool net::tcp::forwarder::start(idle_t idle, void* user)
{
  // If upstream addresses have been defined...
  if (_M_upstream_addresses.count() > 0) {
    // For each worker thread...
    for (size_t i = 0; i < _M_nworkers; i++) {
      // Start.
      if (!_M_workers[i].start(i, &_M_upstream_addresses, idle, user)) {
        return false;
      }
    }

    return true;
  }

  return false;
}

void net::tcp::forwarder::stop()
{
  // For each worker thread...
  for (size_t i = 0; i < _M_nworkers; i++) {
    // Stop.
    _M_workers[i].stop();
  }
}
