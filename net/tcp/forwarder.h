#ifndef NET_TCP_FORWARDER_H
#define NET_TCP_FORWARDER_H

#include <stdint.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "net/tcp/listeners.h"
#include "net/tcp/connections.h"
#include "net/socket/addresses.h"

namespace net {
  namespace tcp {
    // TCP forwarder.
    class forwarder {
      public:
        // Maximum number of worker threads.
        static constexpr const size_t max_workers = 32;

        // Default number of worker threads.
        static constexpr const size_t default_workers = 2;

        // Idle callback.
        typedef void (*idle_t)(size_t, void*);

        // Constructor.
        forwarder(size_t nworkers = default_workers);

        // Destructor.
        ~forwarder();

        // Listen.
        bool listen(const char* address);
        bool listen(const char* address, in_port_t port);
        bool listen(const char* address, in_port_t minport, in_port_t maxport);
        bool listen(const struct sockaddr& addr, socklen_t addrlen);
        bool listen(const socket::address& addr);

        // Add upstream server.
        bool add_upstream_server(const char* address);
        bool add_upstream_server(const char* address, in_port_t port);
        bool add_upstream_server(const struct sockaddr& addr,
                                 socklen_t addrlen);

        bool add_upstream_server(const socket::address& addr);

        // Start.
        bool start(idle_t idle = nullptr, void* user = nullptr);

        // Stop.
        void stop();

      private:
        // Worker thread.
        class worker {
          public:
            // Constructor.
            worker() = default;

            // Destructor.
            ~worker();

            // Listen.
            bool listen(const char* address);
            bool listen(const char* address, in_port_t port);
            bool listen(const char* address, in_port_t minport, in_port_t maxport);
            bool listen(const struct sockaddr& addr, socklen_t addrlen);
            bool listen(const socket::address& addr);

            // Start.
            bool start(size_t nworker,
                       const socket::addresses* upstream_addresses,
                       idle_t idle,
                       void* user);

            // Stop.
            void stop();

          private:
            // Worker number.
            size_t _M_nworker;

            // Socket addresses of the upstream servers.
            const socket::addresses* _M_upstream_addresses;

            // Epoll file descriptor.
            int _M_epollfd = -1;

            // Listeners.
            listeners _M_listeners;

            // Highest file descriptor of the listeners.
            uint64_t _M_maxlistener = 0;

            // Connections.
            connections _M_connections;

            // Idle callback.
            idle_t _M_idle;

            // Pointer to user data.
            void* _M_user;

            // Thread id.
            pthread_t _M_thread;

            // Running?
            bool _M_running = false;

            // Run.
            static void* run(void* arg);
            void run();

            // Process events.
            void process_events(struct epoll_event* events, size_t nevents);

            // Accept connection(s).
            void accept(int listener);

            // Process connection.
            void process(uint32_t events, connection* conn);

            // Connect to the upstream servers.
            bool connect_upstream_servers(connection* conn);

            // Disable copy constructor and assignment operator.
            worker(const worker&) = delete;
            worker& operator=(const worker&) = delete;
        };

        // Socket addresses of the upstream servers.
        socket::addresses _M_upstream_addresses;

        // Worker threads.
        worker _M_workers[max_workers];
        size_t _M_nworkers = 0;

        // Disable copy constructor and assignment operator.
        forwarder(const forwarder&) = delete;
        forwarder& operator=(const forwarder&) = delete;
    };
  }
}

#endif // NET_TCP_FORWARDER_H
