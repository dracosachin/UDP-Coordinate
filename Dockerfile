FROM ubuntu:22.04

RUN apt-get update && apt-get install -y gcc iproute2 net-tools iputils-ping

# Copy over the source code and hostfile
COPY . /usr/src/myapp
WORKDIR /usr/src/myapp

# Compile the program
RUN gcc -o udp_program main.c

# Use ENTRYPOINT to specify the executable
ENTRYPOINT ["./udp_program"]

# Default CMD to pass arguments to the entrypoint (can be overridden)
CMD ["-h", "hostfile.txt"]