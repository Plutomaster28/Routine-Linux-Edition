# Quick Start: Creating Your First Module

This guide will get you from zero to a working module in **under 10 minutes**.

## Option 1: Lua Module (Easiest - 5 minutes)

### Step 1: Create the file
Create `modules/mybot.lua`:

```lua
-- Module information
module_info = {
    name = "mybot",
    version = "1.0.0",
    author = "YourName"
}

-- Commands
commands = {
    -- Simple command
    test = function(channel_id, user_id, args)
        bot.send_message(channel_id, "My first module works!")
    end,
    
    -- Command with arguments
    say = function(channel_id, user_id, args)
        if args == "" then
            bot.send_message(channel_id, "Usage: ~say <message>")
        else
            bot.send_message(channel_id, "You said: " .. args)
        end
    end
}
```

### Step 2: Test it
1. Start the bot: `./routine.exe`
2. In Discord, type: `~test`
3. You should see: "My first module works!"
4. Type: `~say Hello World`
5. You should see: "You said: Hello World"

### Step 3: Make changes
1. Edit `modules/mybot.lua`
2. In Discord, type: `~reload mybot`
3. Changes are live! No restart needed!

**That's it! You have a working Lua module!**

---

## Option 2: C Module (Advanced - 15 minutes)

### Step 1: Create the file
Create `modules/mybot.c`:

```c
#include "module_interface.h"
#include <string.h>
#include <stdio.h>

static const KernelBridge* bridge = NULL;
static void* bot_context = NULL;

// Command implementation
void cmd_test(void* ctx, const char* channel_id, 
              const char* user_id, const char* args) {
    bridge->send_message(bot_context, channel_id, 
                        "My first C module works!");
}

void cmd_reverse(void* ctx, const char* channel_id,
                const char* user_id, const char* args) {
    if (strlen(args) == 0) {
        bridge->send_message(bot_context, channel_id,
                           "Usage: ~reverse <text>");
        return;
    }
    
    // Reverse the string
    size_t len = strlen(args);
    char* reversed = (char*)malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        reversed[i] = args[len - 1 - i];
    }
    reversed[len] = '\0';
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "Reversed: %s", reversed);
    bridge->send_message(bot_context, channel_id, buffer);
    
    free(reversed);
}

// Command registration
static CommandRegistration commands[] = {
    { "test", "Test command", cmd_test },
    { "reverse", "Reverse text", cmd_reverse },
    { NULL, NULL, NULL }  // Terminator
};

// Required exports
ModuleInfo module_get_info(void) {
    ModuleInfo info;
    info.name = "mybot";
    info.version = "1.0.0";
    info.author = "YourName";
    info.description = "My first C module";
    info.api_version = MODULE_API_VERSION;
    info.type = MODULE_TYPE_NATIVE;
    return info;
}

int module_init(const KernelBridge* b, void* ctx) {
    bridge = b;
    bot_context = ctx;
    bridge->log("INFO", "My C module loaded!");
    return 0;
}

void module_shutdown(void) {
    bridge->log("INFO", "My C module unloading...");
}

const CommandRegistration* module_register_commands(void) {
    return commands;
}
```

### Step 2: Compile it

**Windows (MSYS2 UCRT64)**:
```bash
cd modules
gcc -shared -I../include mybot.c -o mybot.dll
```

**Linux**:
```bash
cd modules
gcc -shared -fPIC -I../include mybot.c -o mybot.so
```

### Step 3: Test it
1. Start the bot: `./routine.exe`
2. You should see: "My C module loaded!" in console
3. In Discord, type: `~test`
4. You should see: "My first C module works!"
5. Type: `~reverse Hello`
6. You should see: "Reversed: olleH"

**That's it! You have a working C module!**

---

## Option 3: C++ Module (Advanced - 15 minutes)

Create `modules/mybot.cpp`:

```cpp
#include "module_interface.h"
#include <string>
#include <algorithm>
#include <sstream>

static const KernelBridge* bridge = nullptr;
static void* bot_context = nullptr;

void cmd_test(void* ctx, const char* channel_id, 
              const char* user_id, const char* args) {
    bridge->send_message(bot_context, channel_id,
                        "My first C++ module works!");
}

void cmd_upper(void* ctx, const char* channel_id,
               const char* user_id, const char* args) {
    std::string text(args);
    if (text.empty()) {
        bridge->send_message(bot_context, channel_id,
                           "Usage: ~upper <text>");
        return;
    }
    
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);
    
    std::stringstream ss;
    ss << "**UPPERCASE:** " << text;
    bridge->send_message(bot_context, channel_id, ss.str().c_str());
}

static CommandRegistration commands[] = {
    { "test", "Test C++ module", cmd_test },
    { "upper", "Convert to uppercase", cmd_upper },
    { nullptr, nullptr, nullptr }
};

extern "C" {
    ModuleInfo module_get_info(void) {
        ModuleInfo info;
        info.name = "mybot";
        info.version = "1.0.0";
        info.author = "YourName";
        info.description = "My first C++ module";
        info.api_version = MODULE_API_VERSION;
        info.type = MODULE_TYPE_NATIVE;
        return info;
    }

    int module_init(const KernelBridge* b, void* ctx) {
        bridge = b;
        bot_context = ctx;
        bridge->log("INFO", "My C++ module loaded!");
        return 0;
    }

    void module_shutdown(void) {
        bridge->log("INFO", "My C++ module unloading...");
    }

    const CommandRegistration* module_register_commands(void) {
        return commands;
    }
}
```

**Compile**:
```bash
# Windows (MSYS2)
g++ -shared -std=c++17 -I../include mybot.cpp -o mybot.dll

# Linux
g++ -shared -fPIC -std=c++17 -I../include mybot.cpp -o mybot.so
```

---

## Available Bridge Functions

Your modules can use these functions:

### C/C++ Modules
```c
bridge->send_message(bot_context, channel_id, content);
bridge->log("INFO", "message");  // or "ERROR", "WARN"
uint64_t uptime = bridge->get_uptime(bot_context);
```

### Lua Modules
```lua
bot.send_message(channel_id, content)
bot.log(message)
local uptime = bot.get_uptime()
```

---

## Testing Your Module

1. **List modules**: `~list`
2. **Reload module**: `~reload mybot`
3. **Reload all**: `~reload`
4. **Check status**: `~status`

---

## Common Issues

### Module not loading?
- Check file extension: `.dll` (Windows) or `.so` (Linux)
- Check it's in the `modules/` folder
- Check console for error messages
- Verify API version matches: `MODULE_API_VERSION`

### Command not working?
- Use `~list` to verify module is loaded
- Check command name in Discord
- Check console logs for errors
- Try `~reload mybot` to reload

### Compile errors?
- Include path: `-I../include`
- Use `-shared` flag
- C++: Wrap exports in `extern "C"`
- Windows: Use `.dll` extension
- Linux: Use `.so` and `-fPIC`

---

## Next Steps

1. Create your first module
2. Test basic commands
3. Read [modules/README.md](modules/README.md) for advanced features
4. Check out example modules for inspiration
5. Share your modules with the community!

---

**Pro tip**: Start with Lua for quick prototyping, then rewrite in C/C++ if you need performance!
