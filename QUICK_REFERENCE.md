# Quick reference

## Build and run

```bash
./install_deps_linux.sh
./build_linux.sh
./build-linux/kernel/routine
```

## Kernel commands

- `/help` — complete loaded command catalog
- `/ping` — responsiveness check
- `/echo` — repeat text
- `/bench` — C++ benchmark
- `/version` — kernel build information
- `/uptime` — process uptime
- `/status` — runtime statistics
- `/list` — loaded modules and extensions
- `/reload` — reload a native or Lua module
- `/script` — manage scripts

Legacy `~` forms remain available when Message Content Intent is configured.
The bundled economy commands are documented in
[ECONOMY_GAME.md](ECONOMY_GAME.md).

## Runtime locations

- `config.json` — private bot configuration
- `data/` — private persistent state
- `modules/` — auto-loaded command modules
- `lib/` — auto-loaded low-level extensions
- `examples/modules/` — tutorial sources, not auto-loaded
- `build-linux/kernel/routine` — release executable

## Configuration

```json
{
  "bot_token": "YOUR_BOT_TOKEN_HERE",
  "application_id": "",
  "guild_id": "",
  "slash_commands": {
    "enabled": true,
    "register_on_start": true,
    "guild_id": "YOUR_TEST_GUILD_ID"
  },
  "log_level": "info",
  "reconnect_attempts": 5,
  "heartbeat_interval": 41250,
  "gateway_intents": 37377
}
```

Use a slash-command guild ID while developing and clear it for global
registration. `application_id` can remain empty.

## Module languages

| Language | Runtime form |
|---|---|
| Lua | `.lua` |
| C/C++ | native `.so` |
| Fortran | native `.so` |
| Assembly | architecture-specific `.so` |

Native modules implement [module_interface.h](include/module_interface.h).
Extensions implement [extension_interface.h](include/extension_interface.h).

## Validation

```bash
./build_linux.sh
./release_audit.sh
git diff --check
```

## Documentation

- [Quick start](QUICKSTART.md)
- [Slash commands](SLASH_COMMANDS.md)
- [Module system](MODULE_SYSTEM.md)
- [Linux portability](LINUX_PORTABILITY.md)
- [Public release](PUBLIC_RELEASE.md)
