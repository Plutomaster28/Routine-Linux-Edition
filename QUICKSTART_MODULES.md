# Linux module quick start

Routine loads production modules from `modules/`. Tutorial sources live in
`examples/modules/` so they do not consume slash-command slots or run
accidentally.

## Choose a module type

| Type | Runtime file | Best for |
|---|---|---|
| Lua | `.lua` | Fast iteration and lightweight commands |
| C | `.so` | Small native integrations and stable ABI work |
| C++ | `.so` | Larger native features and standard-library use |
| Fortran | `.so` | Numerical routines behind the C ABI |

Native modules execute inside the bot process. Only load code you trust.

## Lua module

Create `modules/hello.lua`:

```lua
module_info = {
    name = "hello",
    version = "1.0.0",
    author = "you"
}

commands = {
    hello = function(channel_id, user_id, args)
        local suffix = args ~= "" and (", " .. args) or ""
        bot.send_message(channel_id, "Hello <@" .. user_id .. ">" .. suffix)
    end
}

function on_load()
    bot.log("INFO", "hello module loaded")
end

function on_unload()
    bot.log("INFO", "hello module unloading")
end
```

Run the normal build and bot:

```bash
./build_linux.sh
./build-linux/kernel/routine
```

The command is available as `/hello` after the next catalog synchronization
and immediately as `~hello` when legacy prefix transport is enabled. Reload an
edited Lua module with `/reload input: hello` or `~reload hello`.

## Native C module

Copy [example_c_module.c](examples/modules/example_c_module.c) into a new
source file under `modules/`, then change its metadata and command table. A
minimal native module exports:

```c
#include "module_interface.h"

static const KernelBridge* kernel;
static void* bot;

static void hello(void* ignored, const char* channel_id,
                  const char* user_id, const char* args) {
    (void)ignored;
    (void)user_id;
    (void)args;
    kernel->send_message(bot, channel_id, "Hello from native Linux C.");
}

static CommandRegistration commands[] = {
    {"nativehello", "Say hello from a native module", hello},
    {NULL, NULL, NULL}
};

ModuleInfo module_get_info(void) {
    return (ModuleInfo){
        "native_hello", "1.0.0", "you",
        "Minimal Linux native module",
        MODULE_API_VERSION, MODULE_TYPE_NATIVE
    };
}

int module_init(const KernelBridge* bridge, void* context) {
    kernel = bridge;
    bot = context;
    return bridge ? 0 : 1;
}

void module_shutdown(void) {
    kernel = NULL;
    bot = NULL;
}

const CommandRegistration* module_register_commands(void) {
    return commands;
}
```

Add the target to `modules/CMakeLists.txt`:

```cmake
add_library(native_hello SHARED native_hello.c)
target_include_directories(native_hello PRIVATE ${CMAKE_SOURCE_DIR}/../include)
set_target_properties(native_hello PROPERTIES PREFIX "")
```

Then use `./build_linux.sh`. The build installs the resulting `.so` into the
runtime `modules/` directory.

## Native C++ module

Use the same ABI and exports, wrap them in `extern "C"`, and build a shared
library:

```cmake
add_library(my_cpp_module SHARED my_cpp_module.cpp)
target_compile_features(my_cpp_module PRIVATE cxx_std_17)
target_include_directories(my_cpp_module PRIVATE ${CMAKE_SOURCE_DIR}/../include)
set_target_properties(my_cpp_module PROPERTIES PREFIX "")
```

Do not let C++ exceptions cross the C ABI. Catch them inside the callback and
send a safe error response.

## Kernel bridge

Module API version 4 provides:

- `send_message`: enqueue a Discord response;
- `log`: write a module log entry;
- `get_uptime`: read process uptime;
- `get_guild_id`: resolve the guild owning a channel;
- `get_extension_function`: resolve a loaded extension API;
- `get_user_roles`: read cached Discord role IDs;
- `is_guild_admin`: check owner, Administrator, or Manage Server authority.

Callbacks receive the channel ID, invoking user ID, and one compatibility
argument string. Slash commands expose this string as the optional `input`
field. A future ABI may add typed slash-command schemas; do not assume typed
options today.

## Command budget

Discord permits at most 100 global chat-input commands per application. The
production catalog currently uses that budget. Adding a new command may cause
the alphabetically last command to be skipped. Prefer:

- replacing an obsolete alias;
- adding subcommands inside an existing command;
- disabling an optional module;
- developing with a reduced module set.

## Testing checklist

1. Build with `./build_linux.sh`.
2. Confirm the module appears in `/list`.
3. Confirm startup reports the expected registered command.
4. Exercise slash and legacy forms if both transports matter.
5. Test empty, malformed, maximum-size, and unauthorized input.
6. Send a long response and let the kernel's message chunker handle it.
7. Stop with `Ctrl+C` and confirm the module unloads cleanly.

For the complete ABI reference, see [MODULE_SYSTEM.md](MODULE_SYSTEM.md) and
[modules/README.md](modules/README.md).
