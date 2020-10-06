#ifndef NET_TCP_CONNECTION_H
#define NET_TCP_CONNECTION_H

#include <stdint.h>
#include "string/buffer.h"

namespace net {
  namespace tcp {
    // Forward declaration.
    class connections;

    // TCP connection.
    class connection {
      friend class connections;

      public:
        // Constructor.
        connection(connections& connections);

        // Destructor.
        ~connection();

        // Initialize.
        void init(int fd);

        // Close connection.
        void close();

        // Process events.
        void process_events(uint32_t events);

        // Add client connection.
        void add_client(connection* client);

        // Remove server connection and its client connections.
        void remove_server();

        // Is the connection open?
        bool is_open() const;

      private:
        // Maximum buffer size.
        static constexpr const size_t max_buffer_size = 1024 * 1024;

        // Connections.
        connections& _M_connections;

        // Socket descriptor.
        int _M_fd = -1;

        // Is the socket readable?
        bool _M_readable;

        // Is the socket writable?
        bool _M_writable;

        // Is the socket connected to the upstream server?
        bool _M_connected;

        // Buffer.
        string::buffer _M_buf;

        // Node.
        struct node {
          union {
            connection* prev;
            connection* first;
          };

          union {
            connection* next;
            connection* last;
          };
        };

        node _M_node;

        // Pointer to the server connection.
        connection* _M_server;

        // For server connections:
        //   * Pointer to the first and last client connections.
        // For client connections:
        //   * Pointer to the previous and next client connections.
        node _M_client;

        // Read from the server connection.
        // Returns:
        //   true: the server connection shouldn't be removed.
        //         There are two possible cases:
        //           * The server connection is still active and there is, at
        //             least, one client connection.
        //           * The server connection has been already removed because
        //             there are no more client connections.
        //   false: the server connection and the client connections should be
        //          removed.
        bool read();

        // Write.
        bool write(const void* buf, size_t len);
        bool write();

        // Send.
        ssize_t send(const void* buf, size_t len);

        // Remove client connection.
        // If this is the last client connection of the server, the server
        // connection is also removed.
        void remove_client();

        // Disable copy constructor and assignment operator.
        connection(const connection&) = delete;
        connection& operator=(const connection&) = delete;
    };

    inline connection::connection(connections& connections)
      : _M_connections(connections)
    {
    }

    inline bool connection::is_open() const
    {
      return (_M_fd != -1);
    }
  }
}

#endif // NET_TCP_CONNECTION_H
