#include <stdlib.h>
#include <new>
#include "net/tcp/connections.h"
#include "net/tcp/connection.h"

net::tcp::connections::~connections()
{
  erase(_M_connections);
  erase(_M_free);
  erase(_M_firsttemp);
}

net::tcp::connection* net::tcp::connections::pop()
{
  // Allocate connection (if needed).
  if (allocate()) {
    connection* conn = _M_free;

    _M_free = _M_free->_M_node.next;

    conn->_M_node.prev = nullptr;
    conn->_M_node.next = _M_connections;

    if (_M_connections) {
      _M_connections->_M_node.prev = conn;
    }

    _M_connections = conn;

    _M_nconnections++;

    return conn;
  }

  return nullptr;
}

void net::tcp::connections::push(connection* conn)
{
  // Unlink connection.
  unlink(conn);

  // Add temporary connection.
  add_temporary(conn);

  // Decrement number of connections in use.
  _M_nconnections--;
}

void net::tcp::connections::release_temporary()
{
  // If there are temporary connections...
  if (_M_firsttemp) {
    _M_lasttemp->_M_node.next = _M_free;
    _M_free = _M_firsttemp;

    _M_firsttemp = nullptr;
    _M_lasttemp = nullptr;
  }
}

void net::tcp::connections::unlink(connection* conn)
{
  // If not the first connection...
  if (conn->_M_node.prev) {
    conn->_M_node.prev->_M_node.next = conn->_M_node.next;
  } else {
    _M_connections = conn->_M_node.next;
  }

  // If not the last connection...
  if (conn->_M_node.next) {
    conn->_M_node.next->_M_node.prev = conn->_M_node.prev;
  }
}

void net::tcp::connections::add_temporary(connection* conn)
{
  conn->_M_node.next = _M_firsttemp;
  _M_firsttemp = conn;

  // If this is the first temporary connection...
  if (!_M_lasttemp) {
    _M_lasttemp = conn;
  }
}

void net::tcp::connections::erase(const connection* conn)
{
  while (conn) {
    const connection* const next = conn->_M_node.next;

    delete conn;

    conn = next;
  }
}

bool net::tcp::connections::allocate()
{
  // If there are free connections...
  if (_M_free) {
    return true;
  } else {
    // Compute the maximum number of connections which could be allocated.
    const size_t max = max_connections - _M_nconnections;

    for (size_t i = (allocation <= max) ?  allocation : max; i > 0; i--) {
      // Create new connection.
      connection* conn = new (std::nothrow) connection(*this);

      // If the connection could be created...
      if (conn) {
        conn->_M_node.next = _M_free;
        _M_free = conn;
      } else {
        break;
      }
    }

    return (_M_free != nullptr);
  }
}
