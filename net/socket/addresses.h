#ifndef NET_SOCKET_ADDRESSES_H
#define NET_SOCKET_ADDRESSES_H

#include "net/socket/address.h"

namespace net {
  namespace socket {
    // Socket addresses.
    class addresses {
      public:
        // Constructor.
        addresses() = default;

        // Destructor.
        ~addresses();

        // Add socket address.
        bool add(const char* address);
        bool add(const char* address, in_port_t port);
        bool add(const char* address, in_port_t minport, in_port_t maxport);
        bool add(const struct sockaddr& addr, socklen_t addrlen);
        bool add(const socket::address& addr);

        // Get socket address.
        const socket::address* address(size_t idx) const;

        // Get count.
        size_t count() const;

        // Clear.
        void clear();

      private:
        // Allocation.
        static constexpr const size_t allocation = 8;

        // Socket addresses.
        socket::address* _M_addresses = nullptr;
        size_t _M_size = 0;
        size_t _M_used = 0;

        // Allocate socket addresses.
        bool allocate();

        // Disable copy constructor and assignment operator.
        addresses(const addresses&) = delete;
        addresses& operator=(const addresses&) = delete;
    };

    inline const socket::address* addresses::address(size_t idx) const
    {
      return (idx < _M_used) ? &_M_addresses[idx] : nullptr;
    }

    inline size_t addresses::count() const
    {
      return _M_used;
    }

    inline void addresses::clear()
    {
      _M_used = 0;
    }
  }
}

#endif // NET_SOCKET_ADDRESSES_H
