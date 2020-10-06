#ifndef NET_SOCKET_ADDRESS_H
#define NET_SOCKET_ADDRESS_H

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace net {
  namespace socket {
    // Socket address.
    class address {
      public:
        // Constructor.
        address() = default;
        address(const struct sockaddr& addr, socklen_t addrlen);

        // Destructor.
        ~address() = default;

        // Build.
        bool build(const char* address);
        bool build(const char* address, in_port_t port);

        // Cast operators.
        operator const struct sockaddr&() const;
        operator const struct sockaddr*() const;

        // Get address length.
        socklen_t length() const;

        // To string.
        const char* to_string(char* dst, size_t size) const;

      private:
        // Address.
        struct sockaddr_storage _M_addr;

        // Address length.
        socklen_t _M_length;

        // Extract IP and port.
        static bool extract_ip_port(const char* address,
                                    char* ip,
                                    in_port_t& port);

        // Parse port.
        static bool parse_port(const char* s, in_port_t& port);
    };

    inline address::address(const struct sockaddr& addr, socklen_t addrlen)
      : _M_length(addrlen)
    {
      memcpy(&_M_addr, &addr, addrlen);
    }

    inline address::operator const struct sockaddr&() const
    {
      return reinterpret_cast<const struct sockaddr&>(_M_addr);
    }

    inline address::operator const struct sockaddr*() const
    {
      return reinterpret_cast<const struct sockaddr*>(&_M_addr);
    }

    inline socklen_t address::length() const
    {
      return _M_length;
    }
  }
}

#endif // NET_SOCKET_ADDRESS_H
