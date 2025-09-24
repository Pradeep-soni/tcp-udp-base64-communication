# TCP/UDP Communication System with Base64 Encoding (C)

## 📌 Overview

This project implements a **client–server communication system in C** that supports both **TCP** (connection-oriented) and **UDP** (connectionless) protocols. Messages are **Base64-encoded** by the client before transmission and **decoded** by the server upon reception. The server can handle multiple TCP clients concurrently using **POSIX threads (pthreads)** and manages both TCP and UDP sockets simultaneously using **I/O multiplexing** (`select()`).

## 🚀 Features

- **Dual Protocol Support**: Communication over both TCP and UDP
- **Custom Message Protocol**:
  - Type 1 → Base64-encoded message
  - Type 2 → Acknowledgment (ACK)
  - Type 3 → Termination (for TCP clients)
- **Base64 Encoding/Decoding**: Ensures safe transfer of messages as text
- **Multithreaded TCP Server**: Uses pthreads to handle multiple clients concurrently
- **I/O Multiplexing**: Uses `select()` to monitor both TCP and UDP sockets efficiently
- **Graceful Termination**: Clients can exit cleanly with a `quit` command

## 🛠️ Technologies Used

- **C Programming**
- **Socket Programming (TCP & UDP)**
- **POSIX Threads (pthreads)**
- **I/O Multiplexing with `select()`**
- **Base64 Encoding/Decoding**

## 📂 Project Structure

```
tcp-udp-base64-comm/
├── server.c        # Server implementation
├── client.c        # Client implementation
├── README.md       # Project documentation
└── report.pdf      # (Optional) Assignment report
```

## ⚙️ Compilation & Execution

### 1. Compile

```bash
# Compile server
gcc -o server server.c -lpthread

# Compile client
gcc -o client client.c
```

### 2. Run the Server

```bash
./server <port>
# Example:
./server 8888
```

### 3. Run the Client

```bash
./client <server_ip> <port> <protocol>
# Example (TCP):
./client 127.0.0.1 8888 tcp

# Example (UDP):
./client 127.0.0.1 8888 udp
```

## 💬 Example Workflow

### 1. Start server:
```bash
./server 8888
```

### 2. Start client (TCP):
```bash
./client 127.0.0.1 8888 tcp
```

### 3. Client Input:
```
Enter message: Hello
```
- Client encodes → `SGVsbG8=`
- Sends Type 1 message

### 4. Server Output:
```
[TCP Client 127.0.0.1:XXXXX]
Type 1: Base64-encoded message
Encoded: SGVsbG8=
Decoded: Hello
Sent Type 2: ACK
```

### 5. Client Output:
```
Base64 encoded message: SGVsbG8=
Message Type: Type 2: Acknowledgment (ACK)
Server response: ACK
Received acknowledgment from server
```

### 6. Quit Session:
- Client types `quit` → sends Type 3 termination (for TCP)
- Server closes connection
