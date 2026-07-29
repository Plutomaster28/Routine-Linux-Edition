# Platform status

Routine contains portability abstractions for Windows, Linux, and macOS, but
this repository's release target is native Linux.

| Platform | Build status | Runtime status |
|---|---|---|
| Linux x86_64 | Tested | Tested |
| Linux ARM64/ARM32 | Supported by CMake; CI pending | Runtime validation pending |
| Windows UCRT64 | Legacy implementation exists | New kernel changes need retesting |
| macOS | Unix abstractions exist | Build and runtime validation pending |

The former Windows-oriented Routine codebase was the starting point. This
edition replaced mixed MSYS2/native dependency discovery, unsafe shutdown and
threading assumptions, and non-portable runtime loading behavior with the
Linux-tested implementations documented in
[LINUX_PORTABILITY.md](LINUX_PORTABILITY.md).

## Portable kernel areas

- Standard C++17 filesystem, threading, and synchronization
- Platform-specific `dlopen`/`LoadLibrary` branches
- Portable C module and extension boundaries
- libcurl and libwebsockets networking
- Scalar fallbacks for architecture-specific SIMD

## Before claiming another supported platform

1. Install that platform's native dependency set.
2. Build the kernel, every module, and every extension.
3. Run all CTest targets.
4. Exercise Gateway connect/reconnect and Ctrl+C shutdown.
5. Verify slash-command registration and interaction follow-ups.
6. Test module reload and ABI rejection.
7. Run sanitizer or platform-equivalent memory diagnostics.

Until those checks pass in CI and a real Discord session, documentation should
describe the platform as unverified rather than fully supported.
