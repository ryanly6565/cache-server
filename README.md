# CacheServer

CacheServer is a multi-threaded in-memory key-value server written in C++20. It can handle parallel TCP connections from multiple clients and supports operations like accessing, deleting, and expiring string values.

## Features

- TCP server using POSIX sockets
- Multiple parallel clients
- Thread-safe shared key-value storage
- Automated key expiration
- Least recently used key eviction
- Case-insensitive commands
- Values containing spaces
- Request-size protection
- Controlled server shutdown
- Unit and TCP integration tests via GoogleTest
- Configurable ports, cache capacity, number of worker threads, and maximum queue size for clients

## Supported Commands

| Command | Description | Example |
| --- | --- | --- |
| `SET key value` | Creates or replaces a value associated with key. | `SET username new_user` |
| `GET key` | Retrieves a value based on the key. | `GET username` |
| `DELETE key` | Deletes a key-value pairing based on the key. | `DELETE username` |
| `EXISTS key` | Checks whether the pairing associated with the key exists. | `EXISTS username` |
| `EXPIRE key seconds` | Assigns an expiration time. | `EXPIRE username 60` |
| `TTL key` | Returns the amount of seconds left before the key expires. | `TTL username` |
| `STATS` | Returns cache statistics, like number of hits, misses, etc. | `STATS` |

Commands are case-insensitive, while keys are case-sensitive. Every request must end with a newline.

## Example

```text
SET username new_user
OK

GET username
VALUE new_user

EXISTS username
INTEGER 1

DELETE username
INTEGER 1

GET username
NOT_FOUND
```

## Expiration Behaviour

```text
SET username Ryan
EXPIRE username 60
```

The key expires approximately 60 seconds after the `EXPIRE` command.

- Expired keys behave like missing keys.
- `SET` clears the key's previous expiration.
- `DELETE` removes both the value and its expiration.
- `EXPIRE` fails when the key does not exist.
- Expiration durations must be positive integers.

## Requirements

- Linux/WSL
- CMake 3.20+
- A C++20-compatible compiler
- Internet access to download GoogleTest

## Building

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

The server executable will be located at:

```text
build/cache_server
```

## Running

To start the server on the default port, `8080`:

```bash
./build/cache_server
```

To specify a different port:

```bash
./build/cache_server 9000
```

To specify a different maximum cache capacity:

```bash
./build/cache_server 9000 1000
```

To specify a different number of worker threads:

```bash
./build/cache_server 9000 1000 8
```

To specify a different queue capacity:

```bash
./build/cache_server 9000 1000 8 64
```

Valid port numbers must range from `1` to `65535`.

## Connecting

To connect with Netcat:

```bash
nc 127.0.0.1 8080
```

Then enter commands:

```text
SET username Ryan
GET username
EXISTS username
DELETE username
```

Press `Ctrl+C` to disconnect the client.

## Testing

To build and run the test suite:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

To run the tests repeatedly:

```bash
ctest --test-dir build --output-on-failure --repeat until-fail:10
```

The test suite includes:

- Store unit tests
- Request parser unit tests
- Command processor tests
- Parser to processor pipeline tests
- Real TCP socket integration tests
- Multiple client tests
- Fragmented-send tests
- Oversized request tests
- Server shutdown tests
- Least recently used eviction tests
- Thread pool tests

## Architecture

CacheServer is divided into the following components:

- **Server** — Opens the listening socket, accepts clients, manages client commands, and handles shutdown.
- **RequestParser** — Converts incoming text requests into structured commands and reports syntax errors.
- **CommandProcessor** — Executes parsed commands and produces protocol responses.
- **Store** — Maintains shared key-value pairs, handles expiration and LRU eviction, and protects data from unsafe concurrent access.
- **Thread Pool** — Maintains a bounded client-task queue and a fixed set of worker threads.

## Project Structure

```text
cache-server/
├── CMakeLists.txt
├── include/
│   └── cache_server/
│       ├── command.hpp
│       ├── command_processor.hpp
│       ├── request_parser.hpp
│       ├── server.hpp
│       ├── store.hpp
│       └── thread_pool.hpp
├── src/
│   ├── command_processor.cpp
│   ├── main.cpp
│   ├── request_parser.cpp
│   ├── server.cpp
│   ├── store.cpp
│   └── thread_pool.cpp
└── tests/
    ├── CMakeLists.txt
    ├── command_processor_tests.cpp
    ├── parser_tests.cpp
    ├── request_pipeline_tests.cpp
    ├── server_tests.cpp
    ├── store_tests.cpp
    ├── store_lru_tests.cpp
    └── thread_pool_tests.cpp
```

## V2 Limitations

- Data exists only in memory.
- There is no authentication or encryption.
- The protocol is custom and is not fully Redis-compatible.