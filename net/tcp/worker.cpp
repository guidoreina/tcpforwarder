#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "net/tcp/forwarder.h"
#include "net/tcp/connection.h"

net::tcp::forwarder::worker::~worker()
{
  // Stop worker (if running).
  stop();

  if (_M_epollfd != -1) {
    close(_M_epollfd);
  }
}

bool net::tcp::forwarder::worker::listen(const char* address)
{
  return _M_listeners.listen(address);
}

bool net::tcp::forwarder::worker::listen(const char* address, in_port_t port)
{
  return _M_listeners.listen(address, port);
}

bool net::tcp::forwarder::worker::listen(const char* address,
                                         in_port_t minport,
                                         in_port_t maxport)
{
  return _M_listeners.listen(address, minport, maxport);
}

bool net::tcp::forwarder::worker::listen(const struct sockaddr& addr,
                                         socklen_t addrlen)
{
  return _M_listeners.listen(addr, addrlen);
}

bool net::tcp::forwarder::worker::listen(const socket::address& addr)
{
  return _M_listeners.listen(addr);
}

bool
net::tcp::forwarder::worker::start(size_t nworker,
                                   const socket::addresses* upstream_addresses,
                                   idle_t idle,
                                   void* user)
{
  // Open epoll file descriptor.
  _M_epollfd = epoll_create1(0);

  // If the epoll file descriptor could be opened...
  if (_M_epollfd != -1) {
    // Register listeners on the epoll instance.
    int fd;
    for (size_t i = 0; (fd = _M_listeners.fd(i)) != -1; i++) {
      struct epoll_event ev;
      ev.events = EPOLLIN | EPOLLET;

      if (static_cast<uint64_t>(fd) > _M_maxlistener) {
        _M_maxlistener = static_cast<uint64_t>(fd);
      }

      ev.data.u64 = fd;
      if (epoll_ctl(_M_epollfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        return false;
      }
    }

    // Save worker number.
    _M_nworker = nworker;

    // Save socket addresses of the upstream servers.
    _M_upstream_addresses = upstream_addresses;

    // Save idle callback.
    _M_idle = idle;

    // Save pointer to user data.
    _M_user = user;

    // Start thread.
    if (pthread_create(&_M_thread, nullptr, run, this) == 0) {
      _M_running = true;
      return true;
    }
  }

  return false;
}

void net::tcp::forwarder::worker::stop()
{
  // If the thread is running...
  if (_M_running) {
    _M_running = false;
    pthread_join(_M_thread, nullptr);
  }
}

void* net::tcp::forwarder::worker::run(void* arg)
{
  static_cast<worker*>(arg)->run();
  return nullptr;
}

void net::tcp::forwarder::worker::run()
{
  static constexpr const int timeout = 250; // Milliseconds.
  static constexpr const int
    maxevents = static_cast<int>(connections::max_connections);

  do {
    struct epoll_event events[maxevents];

    // Wait for event.
    const int ret = epoll_wait(_M_epollfd, events, maxevents, timeout);

    switch (ret) {
      default: // At least one event was returned.
        // Process events.
        process_events(events, static_cast<size_t>(ret));
        break;
      case 0: // Timeout.
        if (_M_idle) {
          _M_idle(_M_nworker, _M_user);
        }

        break;
      case -1: // Error.
        if (errno != EINTR) {
          return;
        }

        break;
    }
  } while (_M_running);
}

void net::tcp::forwarder::worker::process_events(struct epoll_event* events,
                                                 size_t nevents)
{
  // For each event...
  for (size_t i = 0; i < nevents; i++) {
    // Listener?
    if (events[i].data.u64 <= _M_maxlistener) {
      // If the socket is readable...
      if (events[i].events & EPOLLIN) {
        // Accept connection(s).
        accept(events[i].data.fd);
      }
    } else {
      // Process connection.
      process(events[i].events, static_cast<connection*>(events[i].data.ptr));
    }
  }

  // Release temporary connections.
  _M_connections.release_temporary();
}

void net::tcp::forwarder::worker::accept(int listener)
{
  do {
    // Accept connection.
    const int fd = accept4(listener, nullptr, nullptr, SOCK_NONBLOCK);

    // If the connection could be accepted...
    if (fd != -1) {
      // Get new connection.
      connection* const conn = _M_connections.pop();

      if (conn) {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
        ev.data.ptr = conn;

        // Add connection to the epoll file descriptor.
        if (epoll_ctl(_M_epollfd, EPOLL_CTL_ADD, fd, &ev) == 0) {
          // Initialize connection.
          conn->init(fd);

          // Connect to the upstream servers.
          if (!connect_upstream_servers(conn)) {
            // Remove server connection.
            conn->remove_server();
          }
        } else {
          // Return connection to the pool.
          _M_connections.push(conn);

          // Close socket.
          close(fd);
        }
      } else {
        // Close socket.
        close(fd);
      }
    } else if (errno != EINTR) {
      return;
    }
  } while (true);
}

void net::tcp::forwarder::worker::process(uint32_t events, connection* conn)
{
  // If the connection is open...
  if (conn->is_open()) {
    // Process events.
    conn->process_events(events);
  }
}

bool net::tcp::forwarder::worker::connect_upstream_servers(connection* conn)
{
  size_t nclients = 0;

  const socket::address* address;
  for (size_t i = 0;
       (address = _M_upstream_addresses->address(i)) != nullptr;
       i++) {
    const struct sockaddr& addr = static_cast<const struct sockaddr&>(*address);

    // Create socket.
    const int fd = ::socket(addr.sa_family, SOCK_STREAM | SOCK_NONBLOCK, 0);

    // If the socket could be created...
    if (fd != -1) {
      // Connect to the upstream server.
      do {
        if ((connect(fd, &addr, address->length()) == 0) ||
            (errno == EINPROGRESS)) {
          // Get new connection.
          connection* const client = _M_connections.pop();

          if (client) {
            struct epoll_event ev;
            ev.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
            ev.data.ptr = client;

            // Add connection to the epoll file descriptor.
            if (epoll_ctl(_M_epollfd, EPOLL_CTL_ADD, fd, &ev) == 0) {
              // Initialize client connection.
              client->init(fd);

              // Add client connection.
              conn->add_client(client);

              // Increment number of client connections.
              nclients++;
            } else {
              // Return connection to the pool.
              _M_connections.push(client);

              // Close socket.
              close(fd);
            }
          } else {
            // Close socket.
            close(fd);
          }

          break;
        } else if (errno != EINTR) {
          // Close socket.
          close(fd);

          break;
        }
      } while (true);
    }
  }

  return (nclients > 0);
}
