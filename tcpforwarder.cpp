#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <inttypes.h>
#include "net/tcp/forwarder.h"

static void usage(const char* program);
static bool parse_number_workers(int argc,
                                 const char* argv[],
                                 size_t& nworkers);

static bool parse_arguments(int argc,
                            const char* argv[],
                            net::tcp::forwarder& forwarder);

static bool parse_ip_ports(const char* s,
                           char* address,
                           in_port_t& minport,
                           in_port_t& maxport);

static bool parse_number(const char* s,
                         size_t len,
                         const char* name,
                         uint64_t& n,
                         uint64_t min = 0,
                         uint64_t max = ULLONG_MAX);

int main(int argc, const char* argv[])
{
  // Parse number of workers.
  size_t nworkers;
  if (parse_number_workers(argc, argv, nworkers)) {
    net::tcp::forwarder forwarder(nworkers);

    // Parse arguments.
    if (parse_arguments(argc, argv, forwarder)) {
      // Block signals SIGINT and SIGTERM.
      sigset_t set;
      sigemptyset(&set);
      sigaddset(&set, SIGINT);
      sigaddset(&set, SIGTERM);
      if (pthread_sigmask(SIG_BLOCK, &set, nullptr) == 0) {
        // Start TCP forwarder.
        if (forwarder.start()) {
          printf("Waiting for signal to arrive.\n");

          // Wait for signal to arrive.
          int sig;
          while (sigwait(&set, &sig) != 0);

          printf("Signal received.\n");

          forwarder.stop();

          return 0;
        } else {
          fprintf(stderr, "Error starting TCP forwarder.\n");
        }
      } else {
        fprintf(stderr, "Error blocking signals.\n");
      }
    }
  }

  return -1;
}

void usage(const char* program)
{
  fprintf(stderr,
          "Usage: %s "
          "[--bind <ip-port-range>]+ "
          "[--upstream-server <ip-port>]+ "
          "[--number-workers <number-workers>]\n",
          program);

  fprintf(stderr,
          "<ip-port-range> ::= <ip-port> | <ip-address>:<port-range>\n");

  fprintf(stderr, "<ip-port> ::= <ip-address>:<port>\n");
  fprintf(stderr, "<ip-address> ::= <ipv4-address> | <ipv6-address>\n");
  fprintf(stderr, "<port-range> ::= <port>-<port>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Minimum number of workers: 1.\n");

  fprintf(stderr,
          "Maximum number of workers: %zu.\n",
          net::tcp::forwarder::max_workers);

  fprintf(stderr,
          "Default number of workers: %zu.\n",
          net::tcp::forwarder::default_workers);

  fprintf(stderr, "\n");
}

bool parse_number_workers(int argc, const char* argv[], size_t& nworkers)
{
  nworkers = net::tcp::forwarder::default_workers;

  int i = 1;
  while (i < argc) {
    if (strcasecmp(argv[i], "--number-workers") == 0) {
      // If not the last argument...
      if (i + 1 < argc) {
        // Parse number of workers.
        uint64_t n;
        if (parse_number(argv[i + 1],
                         strlen(argv[i + 1]),
                         "number of workers",
                         n,
                         1,
                         net::tcp::forwarder::max_workers)) {
          nworkers = static_cast<size_t>(n);
          return true;
        } else {
          return false;
        }
      } else {
        fprintf(stderr,
                "Expected number of workers after \"--number-workers\".\n");

        return false;
      }
    }

    i++;
  }

  return true;
}

bool parse_arguments(int argc,
                     const char* argv[],
                     net::tcp::forwarder& forwarder)
{
  size_t nbind = 0;
  size_t nupstream = 0;

  int i = 1;
  while (i < argc) {
    if (strcasecmp(argv[i], "--bind") == 0) {
      // If not the last argument...
      if (i + 1 < argc) {
        char address[INET6_ADDRSTRLEN];
        in_port_t minport;
        in_port_t maxport;

        // Parse IP address and port(s).
        if (parse_ip_ports(argv[i + 1], address, minport, maxport)) {
          // If only one port has been specified...
          if (minport == maxport) {
            if (!forwarder.listen(address, minport)) {
              fprintf(stderr, "Error listening on '%s'.\n", argv[i + 1]);
              return false;
            }
          } else {
            if (!forwarder.listen(address, minport, maxport)) {
              fprintf(stderr,
                      "Error listening on address '%s' and ports %u - %u.\n",
                      address,
                      minport,
                      maxport);

              return false;
            }
          }

          // Increment number of bind addresses.
          nbind++;

          i += 2;
        } else {
          return false;
        }
      } else {
        fprintf(stderr, "Expected IP address and port(s) after \"--bind\".\n");
        return false;
      }
    } else if (strcasecmp(argv[i], "--upstream-server") == 0) {
      // If not the last argument...
      if (i + 1 < argc) {
        // Add upstream server.
        if (forwarder.add_upstream_server(argv[i + 1])) {
          // Increment number of upstream servers.
          nupstream++;

          i += 2;
        } else {
          fprintf(stderr, "Error adding upstream server '%s'.\n", argv[i + 1]);
          return false;
        }
      } else {
        fprintf(stderr,
                "Expected upstream server after \"--upstream-server\".\n");

        return false;
      }
    } else if (strcasecmp(argv[i], "--number-workers") == 0) {
      i += 2;
    } else {
      fprintf(stderr, "Invalid argument '%s'.\n", argv[i]);
      return false;
    }
  }

  if (argc > 1) {
    if ((nbind > 0) && (nupstream > 0)) {
      return true;
    } else if (nbind == 0) {
      fprintf(stderr, "At least one bind address has to be specified.\n");
    } else {
      fprintf(stderr, "At least one upstream server has to be specified.\n");
    }
  } else {
    usage(argv[0]);
  }

  return false;
}

bool parse_ip_ports(const char* s,
                    char* address,
                    in_port_t& minport,
                    in_port_t& maxport)
{
  // Search last colon.
  const char* const last_colon = strrchr(s, ':');

  // If the last colon was found...
  if (last_colon) {
    size_t len = last_colon - s;
    if (len > 0) {
      if (*s == '[') {
        if ((len > 2) && (last_colon[-1] == ']')) {
          // Skip '['.
          s++;

          len -= 2;
        } else {
          return false;
        }
      }

      if (len < INET6_ADDRSTRLEN) {
        const char* const port = last_colon + 1;
        const char* const hyphen = strchr(port, '-');

        if (!hyphen) {
          uint64_t n;
          if (parse_number(port, strlen(port), "port", n, 1, 65535)) {
            memcpy(address, s, len);
            address[len] = 0;

            minport = static_cast<in_port_t>(n);
            maxport = minport;

            return true;
          }
        } else {
          uint64_t n1;
          uint64_t n2;
          if ((parse_number(port, hyphen - port, "port", n1, 1, 65535)) &&
              (parse_number(hyphen + 1,
                            strlen(hyphen + 1),
                            "port",
                            n2,
                            1,
                            65535))) {
            if (n1 <= n2) {
              memcpy(address, s, len);
              address[len] = 0;

              minport = static_cast<in_port_t>(n1);
              maxport = static_cast<in_port_t>(n2);

              return true;
            } else {
              fprintf(stderr,
                      "Invalid port range %" PRIu64 " - %" PRIu64 ".\n",
                      n1,
                      n2);
            }
          }
        }
      } else {
        fprintf(stderr,
                "Address '%.*s' is too long.\n",
                static_cast<int>(len),
                s);
      }
    } else {
      fprintf(stderr, "Address is missing (%s).\n", s);
    }
  } else {
    fprintf(stderr, "Port is missing (%s).\n", s);
  }

  return false;
}

bool parse_number(const char* s,
                  size_t len,
                  const char* name,
                  uint64_t& n,
                  uint64_t min,
                  uint64_t max)
{
  // If the string is not empty...
  if (len > 0) {
    uint64_t res = 0;

    for (size_t i = 0; i < len; i++) {
      // Digit?
      if ((s[i] >= '0') && (s[i] <= '9')) {
        const uint64_t tmp = (res * 10) + (s[i] - '0');

        // If the number doesn't overflow...
        if (tmp >= res) {
          // If the number is not too big...
          if (tmp <= max) {
            res = tmp;
          } else {
            fprintf(stderr,
                    "The %s '%.*s' is too big (maximum: %" PRIu64 ").\n",
                    name,
                    static_cast<int>(len),
                    s,
                    max);

            return false;
          }
        } else {
          fprintf(stderr,
                  "Invalid %s '%.*s' (overflow).\n",
                  name,
                  static_cast<int>(len),
                  s);

          return false;
        }
      } else {
        fprintf(stderr,
                "Invalid %s '%.*s' (not a number).\n",
                name,
                static_cast<int>(len),
                s);

        return false;
      }
    }

    // If the number is not too small...
    if (res >= min) {
      n = res;
      return true;
    } else {
      fprintf(stderr,
              "The %s '%.*s' is too small (minimum: %" PRIu64 ").\n",
              name,
              static_cast<int>(len),
              s,
              min);
    }
  } else {
    fprintf(stderr, "The %s is empty.\n", name);
  }

  return false;
}
