#ifndef NET_TCP_CONNECTIONS_H
#define NET_TCP_CONNECTIONS_H

namespace net {
  namespace tcp {
    // Forward declaration.
    class connection;

    // TCP connections.
    class connections {
      public:
        // Maximum number of connections.
        static constexpr const size_t max_connections = 4 * 1024;

        // Constructor.
        connections() = default;

        // Destructor.
        ~connections();

        // Get new connection.
        connection* pop();

        // Return connection.
        void push(connection* conn);

        // Release temporary connections.
        void release_temporary();

      private:
        // Allocation.
        static constexpr const size_t allocation = 256;

        // Connections.
        connection* _M_connections = nullptr;

        // Free connections (connections to be popped out are taken from this
        // list).
        connection* _M_free = nullptr;

        // Temporary connections: they have just been pushed in but cannot be
        // popped out yet.
        connection* _M_firsttemp = nullptr;
        connection* _M_lasttemp = nullptr;

        // Number of connections in use.
        size_t _M_nconnections = 0;

        // Unlink connection.
        void unlink(connection* conn);

        // Add temporary connection.
        void add_temporary(connection* conn);

        // Erase connection list.
        static void erase(const connection* conn);

        // Allocate.
        bool allocate();

        // Disable copy constructor and assignment operator.
        connections(const connections&) = delete;
        connections& operator=(const connections&) = delete;
    };
  }
}

#endif // NET_TCP_CONNECTIONS_H
