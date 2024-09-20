# UDP Heartbeat Coordination Program

## Brief Report on the UDP Heartbeat Coordination Algorithm

### Introduction

The UDP Heartbeat Coordination Algorithm enables multiple processes to communicate with each other to ensure that all participants are ready to begin a task. The coordination is achieved through UDP (User Datagram Protocol), where each process sends and receives "heartbeat" messages. This algorithm is particularly useful in distributed systems where synchronization among multiple components is crucial before performing a coordinated task.

### Algorithm Overview

The algorithm is implemented in a C program where each process:

1. Listens for incoming heartbeat messages from other processes using UDP.
2. Sends periodic heartbeat messages to notify other processes of its presence.
3. Monitors the readiness of all other processes and signals "READY" once it has received heartbeats from all other participants.

The program operates in a Dockerized environment where each process runs in its own container, communicating with others over a shared network.

### Steps of the Algorithm

1. **Initialization**:
   - The program reads the `hostsfile.txt`, which contains the list of hostnames (or containers) that it needs to communicate with.
   - Each process retrieves its hostname (typically from `/etc/hostname` in a Docker environment) and binds a UDP socket to a fixed port.

2. **Heartbeat Message Exchange**:
   - Each process sends a heartbeat message at regular intervals (`HEARTBEAT_INTERVAL`) to all other processes listed in `hostsfile.txt`. The message is of the format: "HEARTBEAT from <hostname>".
   - The program uses `gethostbyname()` to resolve the hostnames from the `hostsfile.txt` into IP addresses, allowing communication between containers.

3. **Receiving Heartbeats**:
   - The process monitors its UDP socket using the `select()` system call, which checks for incoming messages. The `select()` function ensures that the process does not block indefinitely and allows periodic message sending.
   - Upon receiving a heartbeat, the process logs the sender and marks it as "ready." The sender's identity is stored to ensure that heartbeats are only counted once from each process.

4. **Coordination and Readiness**:
   - After receiving a heartbeat from all other processes, the process prints "READY" to indicate that all participants are active and ready.
   - If a process does not receive all heartbeats within a predefined timeout (`TIMEOUT`), the process exits with a failure message.

5. **Concurrency with `select()`**:
   - The use of the `select()` function allows the program to simultaneously send and receive messages without needing multithreading. It enables non-blocking I/O by checking the socket for incoming messages with a timeout.
   - This ensures the program efficiently manages both heartbeat transmissions and receptions, alternating between the two based on system activity.

### Key Concepts

1. **UDP (User Datagram Protocol)**:
   - UDP is a connectionless protocol, meaning that messages (datagrams) are sent without establishing a connection. This makes UDP lightweight and fast, but messages may be lost or received out of order.
   - The algorithm compensates for this by periodically resending heartbeats, ensuring that missed packets do not cause failure.

2. **`select()` System Call**:
   - `select()` allows the program to wait for activity on the UDP socket for incoming messages. If no activity is detected within the heartbeat interval, the process sends a heartbeat.
   - This allows the program to multiplex between sending heartbeats and receiving messages, ensuring efficient coordination.

3. **Docker Environment**:
   - Each process runs in a separate Docker container, and the containers communicate over a Docker network using the hostnames listed in `hostsfile.txt`.
   - Docker provides isolation, but processes can still discover and communicate with each other using the shared network.

### Conclusion

The UDP Heartbeat Coordination Algorithm ensures reliable synchronization between multiple processes in a distributed system. By using heartbeat messages and non-blocking I/O with `select()`, the algorithm achieves coordination without complex multithreading or blocking I/O. Docker provides a robust environment for simulating real-world distributed systems, making this approach scalable and practical for larger applications.

### Future Considerations

- **Error Handling**: Future improvements can add more robust error handling, such as retries for missed heartbeats.
- **Fault Tolerance**: To improve fault tolerance, the system could be enhanced to detect and handle failures in individual processes.

---

## Running the UDP Heartbeat Coordination Program

### Prerequisites

Ensure that you have the following installed:
- [Docker](https://www.docker.com/get-started) 
- [Docker Compose](https://docs.docker.com/compose/install/)

### Building and Running the Program with Docker

#### Step 1: Clone the repository
First, clone the repository containing the UDP Heartbeat Coordination Algorithm:

```bash
git clone <repository_url>
cd udp-heartbeat-coordination
```

#### Step 2: Build the Docker Image

Use the following command to build the Docker image for the program:

```bash
docker build -t udp-heartbeat .
```

This command creates a Docker image tagged as udp-heartbeat from the Dockerfile.

#### Step 3: Create a Docker Network

If you don’t already have a Docker network, create one to allow communication between containers:

```bash
docker network create mynetwork
```

#### Step 4: Run Containers

Run multiple containers (processes) using the Docker image, ensuring each container uses a different hostname and the same network.

For example, to run two containers:

```bash
docker run --name container1 --network mynetwork --hostname container1 udp-heartbeat -h /usr/src/myapp/hostsfile.txt
docker run --name container2 --network mynetwork --hostname container2 udp-heartbeat -h /usr/src/myapp/hostsfile.txt
```

	•	--name: The name of the Docker container.
	•	--network: The Docker network you created in Step 3.
	•	--hostname: The hostname used by the program.
	•	-h /usr/src/myapp/hostsfile.txt: The path to the hostsfile.txt inside the Docker container.

#### Step 5: Verify Communication

Each container will send and receive heartbeat messages. When all processes are ready, the message “READY” will be printed to the console.

### Running the Program with Docker Compose

```bash
docker-compose up
```

#### Verify Output

Check the logs for each container to verify that the heartbeat messages are being sent and received. The program will print "READY" once all processes have communicated and are ready.

#### Shut Down Containers

To stop and remove the containers created by Docker Compose, run:

```bash
docker-compose down
```

This will clean up all the resources used by the containers.

These instructions will help you build and run the UDP Heartbeat Coordination program using Docker and Docker Compose. Make sure the hostsfile.txt contains the hostnames of all the participating containers for correct coordination.
