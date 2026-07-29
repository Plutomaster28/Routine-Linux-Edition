# Routine

Routine is a Linux-native Discord bot kernel written in C++17. It speaks to
Discord directly through libwebsockets and libcurl, loads capabilities at
runtime, and lets one bot combine native C/C++/Fortran modules, Lua modules,
and low-level extensions.

This repository is the independently evolving Linux edition of Routine. The
kernel is intentionally game-agnostic; the bundled local economy is a
first-party module and extension that doubles as a large integration test.

## Highlights

- Native Discord slash commands with automatic startup synchronization
- Gateway interactions acknowledged within Discord's three-second deadline
- Legacy prefix commands retained for compatible deployments
- C-compatible module and extension ABIs
- Native C, C++, Fortran, Lua, and architecture-specific extension support
- Owned REST worker, rate-limit handling, reconnects, and clean shutdown
- Runtime module reloads with ABI and permission validation
- Persistent, atomic local-economy storage with rollback snapshots
- Tested native Linux builds on x86_64

## Build and run

On Debian or Ubuntu:

```bash
./install_deps_linux.sh
cp config.example.json config.json
```

Put the bot token in `config.json`, then:

```bash
./build_linux.sh
./build-linux/kernel/routine
```

`build_linux.sh` builds the kernel, modules, and extensions and runs every
test. See [QUICKSTART.md](QUICKSTART.md) for Discord application setup.

## Slash commands

Routine discovers commands after all modules load and synchronizes them with
Discord using the application-command API. Existing module callbacks require
no Discord webhook code: the kernel defers an interaction, invokes the normal
callback, and routes its response back to the interaction.

```json
{
  "slash_commands": {
    "enabled": true,
    "register_on_start": true,
    "guild_id": ""
  }
}
```

An empty `guild_id` registers global commands. Set it to a development server
ID for immediate, server-scoped updates. `application_id` may be left empty;
Routine learns it from Discord's READY event.

More detail is in [SLASH_COMMANDS.md](SLASH_COMMANDS.md).

## Architecture

```text
Discord Gateway
      │
      ├── MESSAGE_CREATE ───── legacy command handler
      └── INTERACTION_CREATE ─ slash command handler
                                  │
                     ┌────────────┴────────────┐
                     │                         │
                kernel commands          module loader
                                               │
                                  native modules / Lua modules
                                               │
                                        extension ABI
```

- `src/` and `include/` contain the generic kernel.
- `modules/` contains user-facing commands and event handlers.
- `lib/` contains lower-level extension capabilities.
- `examples/modules/` contains tutorial modules that are not auto-loaded.
- `tests/` covers kernel utilities and the bundled economy.
- `data/`, `config.json`, compiled libraries, and logs are runtime-only and
  ignored by Git.

The economy can be removed without changing the kernel: remove its module,
extension, ABI header, tests, and documentation, then remove its artifact
names from `build_linux.sh`.

## Documentation

- [Quick start](QUICKSTART.md)
- [Slash commands](SLASH_COMMANDS.md)
- [Module system](MODULE_SYSTEM.md)
- [Economy game](ECONOMY_GAME.md)
- [Linux portability audit](LINUX_PORTABILITY.md)
- [Public release checklist](PUBLIC_RELEASE.md)
- [Security policy](SECURITY.md)

## Platform status

Linux x86_64 is the release target and fully tested path. Much of the kernel
retains Windows and macOS abstractions, but this edition does not claim those
platforms as verified until their native CI and runtime tests exist. See
[CROSS_PLATFORM.md](CROSS_PLATFORM.md).

## License

Apache License 2.0. See [LICENSE](LICENSE).
