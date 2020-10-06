#ifndef NET_TCP_LISTENERS_H
#define NET_TCP_LISTENERS_H

#include "net/socket/address.h"

namespace net {
  namespace tcp {
    // TCP listeners.
    class listeners {
      public:
        // Constructor.
        listeners() = default;

        // Destructor.
        ~listeners();

        // Listen.
        bool listen(const char* address);
        bool listen(const char* address, in_port_t port);
        bool listen(const char* address, in_port_t minport, in_port_t maxport);
        bool listen(const struct sockaddr& addr, socklen_t addrlen);
        bool listen(const socket::address& addr);

        // Get fd.
        int fd(size_t idx) const;

        // Get count.
        size_t count() const;

      private:
        // Allocation.
        static constexpr const size_t allocation = 8;

        // File descriptors.
        int* _M_fds = nullptr;
        size_t _M_size = 0;
        size_t _M_used = 0;

        // Allocate.
        bool allocate();

        // Disable copy constructor and assignment operator.
        listeners(const listeners&) = delete;
        listeners& operator=(const listeners&) = delete;
    };

    inline bool listeners::listen(const socket::address& addr)
    {
      return listen(static_cast<const struct sockaddr&>(addr), addr.length());
    }

    inline int listeners::fd(size_t idx) const
    {
      return (idx < _M_used) ? _M_fds[idx] : -1;
    }

    inline size_t listeners::count() const
    {
      return _M_used;
    }
  }
}

#endif // NET_TCP_LISTENERS_H
