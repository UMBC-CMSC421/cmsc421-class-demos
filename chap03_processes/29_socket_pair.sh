#!/bin/bash
# Chapter 3: Processes
# Demo 29: A socket pair, both ends on this machine
#
# What's going on
#   A TCP connection is identified by a pair of sockets: (client IP, client port) and
#   (server IP, server port). The server listens on a fixed, well-known port. The
#   client's OS picks the client's port from its ephemeral range for this one
#   connection; on Linux the default range is 32768 to 60999.
#   This script starts a tiny server on 127.0.0.1 port 8421, connects to it, and
#   shows the connection from both sides with ss.
#   127.0.0.1 is "this machine" (loopback), so the client and the server are just
#   two processes on the same computer.
#
# Run
#   ./29_socket_pair.sh          needs python3 and ss (Debian: iproute2)
#
# What to look for
#   Two ESTAB lines for one connection: one from the client's point of view and one
#   from the server's. The server side always uses port 8421; the client's port is
#   different every run and falls inside ip_local_port_range.
#
# Other ways to see it
#   ss -tn                     your own ssh session to the VM: local port 22 is the
#                              server side; the peer port is the client's ephemeral port
#   cat /proc/sys/net/ipv4/ip_local_port_range

# Stop early with a clear message if a tool we need isn't installed.
command -v python3 >/dev/null || { echo "python3 is required"; exit 1; }
command -v ss >/dev/null || { echo "ss is required (sudo apt install iproute2)"; exit 1; }

# The server: a few lines of Python run in the background (the & at the end).
# It makes a socket, binds it to port 8421, listens, accepts one connection,
# and holds it open for 5 seconds so ss has something to show.
# SO_REUSEADDR lets you rerun the script right away without a "port in use" error.
python3 -c '
import socket, time
s = socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", 8421)); s.listen(1)
c, _ = s.accept(); time.sleep(5)' &
# $! is the server's PID. The trap kills the server when this script exits.
server=$!
trap 'kill $server 2>/dev/null' EXIT
# Give the server a moment to start listening.
sleep 0.5

# The client: bash itself connects. Opening /dev/tcp/HOST/PORT is a bash feature
# that makes a TCP connection. The kernel picks our side's port at this point.
exec 3<>/dev/tcp/127.0.0.1/8421      # bash opens a TCP connection on fd 3
sleep 0.3

# Show every TCP socket with 8421 as its source or destination port.
# -t means TCP only, -n means show numbers instead of names.
echo "\$ ss -tn '( sport = :8421 or dport = :8421 )'"
ss -tn '( sport = :8421 or dport = :8421 )'
echo
echo "\$ cat /proc/sys/net/ipv4/ip_local_port_range"
cat /proc/sys/net/ipv4/ip_local_port_range
# Close fd 3, which closes our end of the connection.
exec 3>&-
