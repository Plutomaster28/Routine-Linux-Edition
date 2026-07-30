# Module and extension system

Routine Linux Edition keeps Discord transport separate from feature code.
Modules provide user-facing commands and events; extensions provide named
low-level APIs to native modules.

## Startup order

```text
configuration
    → kernel services
    → extensions from lib/*.so
    → native modules from modules/*.so
    → Lua modules from modules/*.lua
    → application-command catalog generation
    → Discord synchronization
```

Extensions load first so modules can resolve required functions during
`module_init`. Shutdown happens in reverse order.

## Native module ABI

The contract is [module_interface.h](include/module_interface.h). API version 4
requires:

```c
ModuleInfo module_get_info(void);
int module_init(const KernelBridge*, void*);
void module_shutdown(void);
const CommandRegistration* module_register_commands(void);
```

Optional exports receive raw messages or periodic ticks. Command tables are
null-terminated, bounded by the loader, and copied into kernel-owned metadata.
Callbacks receive channel ID, user ID, and one argument string.

The `KernelBridge` exposes message sending, logging, uptime, channel/guild
resolution, extension lookup, cached roles, and guild-admin checks.

## Lua modules

Lua modules define metadata and a `commands` table. They may also expose
lifecycle and raw-message hooks. The Lua runtime opens the standard libraries;
Lua is not a hardened security sandbox. Only load trusted files.

Tutorial Lua lives under `examples/modules/`. Moving it into `modules/` makes
it production code and consumes slash-command budget.

## Extension ABI

The contract is [extension_interface.h](include/extension_interface.h).
Extensions expose metadata, an ABI version, lifecycle callbacks, capability
checks, and named function lookup.

The kernel does not interpret extension-specific functions. A module and
extension share a separate C header, as the economy does through
`economy_extension_api.h`.

## Slash commands

After every production module loads, the kernel combines built-in, native, and
Lua command definitions. It validates names, deduplicates them, adds the
compatibility `input` option where needed, caps the payload at 100 commands,
and bulk-overwrites the chosen guild or global catalog.

Native module ABI v4 does not expose typed options. Modules parse the same
input string for slash and legacy prefix invocations.

## Reload behavior

`/reload input: NAME` and `~reload NAME` can reload a dynamic module. Reloads
are useful for implementation changes, but command additions/removals should
be followed by a process restart so Discord's catalog is regenerated.

Never unload a native library while its callback is executing or while another
thread retains one of its function pointers.

## Building

Use:

```bash
./build_linux.sh
```

Native targets are declared in `modules/CMakeLists.txt` or
`lib/CMakeLists.txt`. Runtime artifacts are `.so` files with no `lib` prefix.
The build tests each layer before copying artifacts into `modules/` and `lib/`.

## Security model

- Native code is fully trusted, in-process code.
- Lua code is trusted embedded code.
- ABI versions reject known-incompatible binaries, not malicious binaries.
- Administrative authorization belongs in the module and should use
  `is_guild_admin`, not user-supplied text.
- Persistent extensions must validate invariants and fail closed on save
  errors.
- Secrets and runtime data never belong in a module source file.

## Development

See:

- [Linux module quick start](QUICKSTART_MODULES.md)
- [Runtime module directory](modules/README.md)
- [Extension directory](lib/README.md)
- [Fortran guide](modules/FORTRAN_GUIDE.md)
- [Slash-command transport](SLASH_COMMANDS.md)
