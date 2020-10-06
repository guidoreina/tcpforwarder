#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include "net/tcp/connection.h"
#include "net/tcp/connections.h"

net::tcp::connection::~connection()
{
  if (_M_fd != -1) {
    ::close(_M_fd);
  }
}

void net::tcp::connection::init(int fd)
{
  // Save socket descriptor.
  _M_fd = fd;

  // Socket is not readable.
  _M_readable = false;

  // Socket is not writable.
  _M_writable = false;

  // Socket is not connected to the upstream server.
  _M_connected = false;

  // Clear buffer.
  _M_buf.clear();

  // Clear pointers.
  _M_server = nullptr;
  _M_client.prev = nullptr;
  _M_client.next = nullptr;
}

void net::tcp::connection::close()
{
  ::close(_M_fd);
  _M_fd = -1;
}

void net::tcp::connection::process_events(uint32_t events)
{
  // If not error...
  if ((events & (EPOLLERR | EPOLLHUP)) == 0) {
    // If the socket is readable (only server connections)...
    if (events & EPOLLIN) {
      // Mark the connection as readable.
      _M_readable = true;

      // Read.
      if ((!read()) || (events & EPOLLRDHUP)) {
        // Remove server and client connections.
        remove_server();
      }
    } else if (events & EPOLLOUT) {
      // The socket is writable (only client connections)...

      // If we are not connected to the upstream server yet...
      if (!_M_connected) {
        // Get socket error.
        int error;
        socklen_t optlen = sizeof(int);
        if ((getsockopt(_M_fd, SOL_SOCKET, SO_ERROR, &error, &optlen) == 0) &&
            (error == 0)) {
          _M_connected = true;
        } else {
          // Remove client connection and, if this is the last client connection
          // of the server, also the server connection.
          remove_client();

          return;
        }
      }

      // Mark the connection as writable.
      _M_writable = true;

      // If the buffer is not empty => write.
      if (((!_M_buf.empty()) && (!write())) || (events & EPOLLRDHUP)) {
        // Remove client connection and, if this is the last client connection
        // of the server, also the server connection.
        remove_client();
      }
    }
  } else {
    // If this is a server connection...
    if (!_M_server) {
      // Remove server and client connections.
      remove_server();
    } else {
      // Remove client connection and, if this is the last client connection of
      // the server, also the server connection.
      remove_client();
    }
  }
}

void net::tcp::connection::add_client(connection* client)
{
  client->_M_server = this;

  client->_M_client.prev = nullptr;
  client->_M_client.next = _M_client.first;

  // If this is not the first connection...
  if (_M_client.first) {
    _M_client.first->_M_client.prev = client;
  } else {
    _M_client.last = client;
  }

  // Make the client connection the first connection.
  _M_client.first = client;
}

void net::tcp::connection::remove_server()
{
  connection* client = _M_client.first;

  // For each client...
  while (client) {
    connection* const next = client->_M_client.next;

    // Close client connection.
    client->close();

    // Return client connection to the pool.
    _M_connections.push(client);

    client = next;
  }

  // Close connection.
  close();

  // Return connection to the pool.
  _M_connections.push(this);
}

bool net::tcp::connection::read()
{
  static constexpr const size_t buffer_size = 32 * 1024;

  do {
    // Receive.
    uint8_t buf[buffer_size];
    const ssize_t ret = ::recv(_M_fd, buf, sizeof(buf), 0);

    switch (ret) {
      default:
        {
          // Make `client` point to the first client.
          connection* client = _M_client.first;

          // Send data to all the clients.
          do {
            // Make `next` point to the next client.
            connection* const next = client->_M_client.next;

            // Send data to the client.
            if (!client->write(buf, ret)) {
              // Remove client connection and, if this is the last client
              // connection of the server, also the server connection.
              client->remove_client();

              // If there are no more client connections...
              if (!_M_client.first) {
                // The connection shouldn't be removed.
                return true;
              }
            }

            client = next;
          } while (client);

          // If we have exhausted the read I/O space...
          if (static_cast<size_t>(ret) < sizeof(buf)) {
            _M_readable = false;

            // The connection shouldn't be removed.
            return true;
          }
        }

        break;
      case 0:
        // Connection closed by peer => remove connection.
        return false;
      case -1:
        if (errno == EAGAIN) {
          _M_readable = false;

          // The connection shouldn't be removed.
          return true;
        } else if (errno != EINTR) {
          // The connection should be removed.
          return false;
        }

        break;
    }
  } while (true);
}

bool net::tcp::connection::write(const void* buf, size_t len)
{
  // If the connection is writable...
  if (_M_writable) {
    // Send.
    const ssize_t ret = send(buf, len);

    // If we could send some data...
    if (ret > 0) {
      // If we could send all the data...
      if (static_cast<size_t>(ret) == len) {
        return true;
      } else {
        buf = static_cast<const uint8_t*>(buf) + ret;
        len -= ret;
      }
    } else {
      if (errno != EAGAIN) {
        return false;
      }
    }
  }

  // If we can still append the data to the buffer...
  if (_M_buf.length() + len <= max_buffer_size) {
    return _M_buf.append(buf, len);
  } else {
    return false;
  }
}

bool net::tcp::connection::write()
{
  // Send.
  const ssize_t ret = send(_M_buf.data(), _M_buf.length());

  // If we could send some data...
  if (ret > 0) {
    _M_buf.erase(0, ret);
    return true;
  } else {
    return (errno == EAGAIN);
  }
}

ssize_t net::tcp::connection::send(const void* buf, size_t len)
{
  do {
    // Send.
    const ssize_t ret = ::send(_M_fd, buf, len, MSG_NOSIGNAL);

    // If we could send some data...
    if (ret > 0) {
      // If we couldn't send all the data...
      if (static_cast<size_t>(ret) < len) {
        _M_writable = false;
      }

      return ret;
    } else if (ret < 0) {
      if (errno == EAGAIN) {
        _M_writable = false;
        return -1;
      } else if (errno != EINTR) {
        return -1;
      }
    }
  } while (true);
}

void net::tcp::connection::remove_client()
{
  // If not the first client connection...
  if (_M_client.prev) {
    _M_client.prev->_M_client.next = _M_client.next;
  } else {
    _M_server->_M_client.first = _M_client.next;
  }

  // If not the last client connection...
  if (_M_client.next) {
    _M_client.next->_M_client.prev = _M_client.prev;
  } else {
    _M_server->_M_client.last = _M_client.prev;
  }

  // Close connection.
  close();

  // Return connection to the pool.
  _M_connections.push(this);

  // If this was the last client connection of the server...
  if (!_M_server->_M_client.first) {
    // Close server connection.
    _M_server->close();

    // Return server connection to the pool.
    _M_connections.push(_M_server);
  }
}
