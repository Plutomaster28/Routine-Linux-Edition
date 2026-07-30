# Runtime modules

Routine Linux Edition loads user-facing behavior from this directory. Lua
sources (`.lua`) and native Linux shared libraries (`.so`) are discovered at
startup. Tutorial sources belong in `../examples/modules/` and are not
auto-loaded.

## Supported forms

- Lua modules define `module_info`, a `commands` table, and optional lifecycle
  or raw-message hooks.
- Native C, C++, Fortran, and assembly modules implement the C ABI in
  `../include/module_interface.h`.
- Native module API version 4 is the current contract.

Native required exports:

```c
ModuleInfo module_get_info(void);
int module_init(const KernelBridge* bridge, void* bot_context);
void module_shutdown(void);
const CommandRegistration* module_register_commands(void);
```

Optional exports:

```c
void module_on_message(void*, const char*, const char*, const char*);
void module_on_tick(void*);
```

## Kernel bridge

The version-4 bridge exposes:

- `send_message`
- `log`
- `get_uptime`
- `get_guild_id`
- `get_extension_function`
- `get_user_roles`
- `is_guild_admin`

The economy module uses this boundary to resolve its simulation extension,
route state by real guild, consume cached Discord roles, and authorize server
configuration.

## Build

Use the repository build instead of compiling ad hoc:

```bash
./build_linux.sh
```

The modules CMake project builds configured C/C++/Fortran targets, runs their
tests, and installs the resulting `.so` files here. To add a native target,
edit `CMakeLists.txt`; see [the module quick start](../QUICKSTART_MODULES.md).

## Loading and reloads

- `/list` shows loaded native and Lua modules.
- `/reload input: NAME` reloads a module.
- Equivalent `~list` and `~reload NAME` forms require legacy message content.
- Restart after adding or removing commands so the slash-command catalog is
  regenerated predictably.

Discord limits one application to 100 global chat-input commands. The bundled
production set uses that budget, so tutorial modules remain outside this
directory by default.

## Bundled production modules

- `local_economy_module.so`: Discord interface for Economy Version 2.
- `fortran_math.so`: small native Fortran integration demonstration.
- `miyamii.lua`, `relay.lua`, and `breakdown.lua`: project-specific Lua
  modules.

## Security

- Every native module executes in the bot process with the bot user's operating
  system privileges. A crash or memory error can take down the kernel.
- Lua calls `luaL_openlibs`; it is convenient, not a hardened sandbox. Treat
  Lua modules as trusted code too.
- Validate lengths, amounts, IDs, and authorization inside every callback.
- Do not hold pointers supplied by a callback past their documented lifetime.
- Do not let C++ exceptions cross an `extern "C"` boundary.
- Keep blocking work off synchronous command and Gateway paths.

## Directory policy

```text
modules/
├── CMakeLists.txt
├── local_economy_module.cpp
├── fortran_math.f90
├── *.lua
├── *.so                 # generated, ignored by Git
└── README.md

examples/modules/
├── example.lua
└── example_c_module.c
```

Source and documentation are public. Built libraries, configuration, logs, and
runtime data remain ignored.
