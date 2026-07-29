# Routine Bot - Modules

This directory contains bot modules that extend the kernel's functionality.

## Module Types

Routine supports multiple module types:

### 1. **Lua Modules** (.lua)
- Easiest to create and modify
- Perfect for simple commands and scripting
- No compilation needed
- Hot-reloadable

**Example:**
```lua
module_info = {
    name = "mymodule",
    version = "1.0.0",
    author = "Your Name"
}

commands = {
    test = function(channel_id, user_id, args)
        bot.send_message(channel_id, "Hello from Lua!")
    end
}
```

### 2. **C Modules** (.dll / .so)
- Maximum performance
- Full system access
- Compiled as shared libraries
- Can be written in C, C++, or even Assembly

**Example:** See `../examples/modules/example_c_module.c`

### 3. **C++ Modules** (.dll / .so)
- Object-oriented design
- STL and modern C++ features
- Same interface as C modules

### 4. **Assembly Modules** (.dll / .so)
- Ultimate performance
- Low-level system control
- Uses same C interface

## Module Structure

### Required Exports (C/C++/Assembly)

All compiled modules must export these functions:

```c
ModuleInfo module_get_info(void);
int module_init(const KernelBridge* bridge, void* bot_context);
void module_shutdown(void);
const CommandRegistration* module_register_commands(void);
```

### Optional Exports

```c
void module_on_message(void*, const char*, const char*, const char*);
void module_on_tick(void*);
```

## Kernel Bridge

The kernel provides these functions to modules:

- `send_message(bot_context, channel_id, content)` - Send Discord message
- `log(level, message)` - Log to console
- `get_uptime(bot_context)` - Get bot uptime in seconds
- `get_guild_id(bot_context, channel_id)` - Resolve a channel's guild; returns
  `NULL` for DMs or unknown channels
- `get_extension_function(bot_context, name)` - Resolve a function exported by
  a loaded kernel extension
- `get_user_roles(bot_context, channel_id, user_id)` - Resolve real Discord
  role IDs cached from the current guild message
- `is_guild_admin(bot_context, channel_id, user_id)` - Check guild ownership,
  Administrator, or Manage Server using Discord role permissions

These bridge additions require module API version 4. They let gameplay modules
preserve per-server isolation, delegate authoritative state to a kernel
extension, apply role-derived mechanics, and protect administrative commands
without trusting command text.

## Building Compiled Modules

### Quick Method: Use Build Scripts

**Windows:**
```bash
# In MSYS2 bash
./build_modules.sh

# Or in PowerShell/CMD
build_modules.bat
```

**Linux:**
```bash
chmod +x build_modules.sh
./build_modules.sh
```

The scripts will automatically:
- Find all `.c`, `.cpp`, and `.asm`/`.s` files
- Compile them as shared libraries
- Place them in the modules folder
- Show you what was built

### Manual Compilation

**Windows (MSYS2 UCRT64):**
```bash
gcc -shared -I../include your_module.c -o your_module.dll
```

**Linux:**
```bash
gcc -shared -fPIC -I../include your_module.c -o your_module.so
```

**C++ Module:**
```bash
g++ -shared -std=c++17 -I../include your_module.cpp -o your_module.dll
```

**Assembly Module:**
```bash
# Windows
nasm -f win64 your_module.asm -o your_module.obj
gcc -shared your_module.obj -o your_module.dll

# Linux
nasm -f elf64 your_module.asm -o your_module.o
gcc -shared your_module.o -o your_module.so
```

## Loading Modules

Modules are automatically loaded from this directory when the bot starts.

**Manual loading:**
- Use `~reload <module_name>` to reload a module
- Use `~list` to see loaded modules

## Module Folder Organization

```
modules/
├── example.lua              # Example Lua module
├── ../examples/modules/     # Tutorial sources (not auto-loaded)
├── mymodule.dll            # Compiled module (Windows)
├── mymodule.so             # Compiled module (Linux)
└── README.md               # This file
```

## Examples Included

- **example.lua** - Lua module with commands: ~greet, ~calc, ~random, ~luainfo
- **../examples/modules/example_c_module.c** - C command-module tutorial
- **local_economy_module.cpp** - Full-chaos persistent local economy game;
  see `../ECONOMY_GAME.md`

## Best Practices

1. **Always validate input** - Never trust user input
2. **Handle errors gracefully** - Don't crash the kernel
3. **Keep it lightweight** - Modules should be fast
4. **Document your commands** - Use clear descriptions
5. **Test before deploying** - Test in development first

## Security Notes

- Modules run with full bot permissions
- Only load trusted modules
- Lua modules are sandboxed (safer)
- C/C++ modules have full system access

## Getting Started

1. Copy `example.lua` and modify it for your needs
2. Or write a C module using `../examples/modules/example_c_module.c` as template
3. Place the module in this folder
4. Restart bot or use `~reload` command

---

*Routine Bot - Pure C++ kernel with ultimate extensibility* 🚀
