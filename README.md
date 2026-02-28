# Silber

A lightweight, cross-platform C++ library for inter-process communication using shared memory and semaphores.

## Features

- **Cross-platform**: Linux, macOS, and Windows support
- **Multi-client**: Up to 8 concurrent clients per server
- **Type-safe**: Template-based message API
- **Real-time ready**: Non-blocking operations with timeout support
- **Zero-copy**: Direct shared memory access for performance

## Quick Start

### Building

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Usage

- **Server**: Create `ServerProcCommunicator` to receive requests and send responses
- **Client**: Create `ClientProcCommunicator` to send requests and receive responses
- **Blocking mode**: Wait indefinitely for response
- **Non-blocking mode**: Specify timeout in milliseconds

## Architecture

```
┌─────────┐                    ┌─────────┐
│ Client1 │───┐            ┌───│ Client2 │
└─────────┘   │            │   └─────────┘
              ▼            ▼
         ┌─────────────────────┐
         │   Shared Memory     │
         │  (32KB partitioned) │
         └─────────────────────┘
                   ▲
                   │
              ┌────────┐
              │ Server │
              └────────┘
```

- **Shared memory**: 32KB total (8 × 4KB per client)
- **Synchronization**: 5 semaphores coordinate message flow
- **Communication**: Bidirectional request-response pattern

## Configuration

- **Per-client memory**: 4KB (configurable in `Message.h`)
- **Max clients**: 8 concurrent connections (configurable in `Message.h`)
- **Custom messages**: Extend base `Message` struct with your data

## Platform Support

| Platform | Compiler | Status |
|----------|----------|--------|
| Linux    | GCC 7+   | ✅ Tested |
| macOS    | Clang    | ✅ Tested |
| Windows  | MSVC     | ✅ Tested |

## License

GNU Affero General Public License v3.0 - see [LICENSE](LICENSE)

## Contributing

Contributions welcome! Please ensure CI passes on all platforms before submitting PRs.
