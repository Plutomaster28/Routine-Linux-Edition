# Linux Portability Status

Routine 1.0 is supported as a native Linux build. The verified reference
environment is x86_64 Linux with GCC 13, CMake/Ninja, libwebsockets 4.3,
OpenSSL 3, libcurl 8, Lua 5.4, yaml-cpp 0.8, and gfortran 13.

## Build and Run

```bash
./build_linux.sh
./build-linux/kernel/routine
```

The executable locates the project root from its own path, so it can also be
launched from another working directory. `config.json` is forced to mode
`0600`, generated libraries are installed from an explicit allowlist, and
runtime state remains under the project `data/` directory.

Discord must have **Message Content Intent** enabled for the application that
owns the configured token. The default gateway intent mask is `33281`
(`GUILDS`, `GUILD_MESSAGES`, and `MESSAGE_CONTENT`).

## Portability Hardening

- Native dependencies are resolved through Linux `pkg-config`, avoiding
  accidental linkage to inherited MSYS2/UCRT64 paths.
- The websocket context is serviced and destroyed on a safe thread lifecycle.
  Cross-thread writes wake the service loop instead of touching a live
  `lws*`, fragmented messages are reassembled, close codes are decoded, and
  recoverable disconnects use bounded backoff.
- POSIX signals only set `sig_atomic_t`; shutdown and C++ destruction happen
  from normal threads.
- Discord REST work uses an owned queue and worker rather than detached
  threads. Requests have HTTPS-only protocol policy, no-signal mode, connect
  and total timeouts, and case-insensitive rate-limit header handling.
- Native and Lua module dispatch, ticks, reloads, and unloads are serialized.
  Dynamic libraries use eager local symbol resolution and validate metadata,
  API versions, API tables, command terminators, and callbacks.
- The legacy Fortran extension sample is no longer packaged as a runtime
  extension because it predates the current ExtensionAPI table ABI. The
  ABI-compatible Fortran command module remains enabled.
- The SIMD extension uses SSE/SSE2 on x86 and safe scalar fallbacks on other
  Linux architectures.
- Module reload requires guild ownership, Administrator, or Manage Server.

## Verification

- Clean kernel, extension, and module builds with GCC warning flags
- Economy transaction, persistence, routing, and corrupted-save recovery tests
- AddressSanitizer and UndefinedBehaviorSanitizer builds and test execution
- Sanitized live TLS/gateway connect, fatal close handling, and complete unload
- Runtime ELF dependency and undefined-symbol checks
- Launch from outside the repository with successful root/config/module discovery

LeakSanitizer itself cannot run inside the current ptrace-managed development
sandbox; AddressSanitizer and UndefinedBehaviorSanitizer completed with leak
detection disabled. A production soak test should still be performed after the
Discord Message Content Intent is enabled so the gateway can remain at READY
across multiple heartbeat intervals.
