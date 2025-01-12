# Echo Server and Client

## Overview
This project implements an Echo Server and Client in C++. It facilitates message exchange between a client and a server while demonstrating efficient handling of pipelined requests and concurrent connections using modern, event-driven architecture.

## Features

### Server
- Manages multiple clients simultaneously using an event loop.
- Supports pipelined requests from individual client connections.
- Operates efficiently through non-blocking mechanisms.

### Client
- Sends messages to the server and receives echoed responses.

## Requirements
- **Operating System**: macOS (relies on `kqueue` for the event loop)
- **Language Standard**: C++17 or later
- **Build Tools**: `make` utility, GCC/Clang compiler

## Installation and Compilation

To build the project, use the provided `Makefile`:
```bash
make
```
This will generate the following executables:
- `server`
- `client`

## Usage

### Running the Server
To start the server, execute:
```bash
./server
```
The server listens for incoming client connections on a predefined port.

### Running the Client
To start the client, execute:
```bash
./client
```
The client sends messages to the server, which echoes them back. The output follows this format:
```bash
length: <length of message> res: <message echoed>
```

## Makefile Targets
- **`make` or `make all`**: Compiles the `server` and `client` executables.
- **`make clean`**: Removes all compiled files and executables.

## Project Structure
```
.
├── Makefile          # Build script
├── server.cc         # Server implementation
├── client.cc         # Client implementation
└── README.md         # Documentation
```

## Cleaning Up
To remove compiled files and executables, run:
```bash
make clean
```
