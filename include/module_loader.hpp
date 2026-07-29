#ifndef MODULE_LOADER_HPP
#define MODULE_LOADER_HPP

#include "module_interface.h"
#include "discord_bot.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace discord {

// Forward declaration
class DiscordBot;

// Represents a loaded module
class LoadedModule {
    friend class ModuleLoader;  // Allow ModuleLoader to access private members
    
public:
    LoadedModule(const std::string& path, const std::string& name);
    ~LoadedModule();

    bool load(DiscordBot* bot);
    void unload();
    
    bool is_loaded() const { return loaded_; }
    std::string get_name() const { return name_; }
    ModuleInfo get_info() const { return info_; }
    std::vector<std::string> get_commands() const;
    std::vector<std::pair<std::string, std::string>>
    get_command_definitions() const;

private:
    std::string path_;
    std::string name_;
    bool loaded_;
    ModuleInfo info_;
    
#ifdef _WIN32
    HMODULE handle_;
#else
    void* handle_;
#endif

    // Function pointers to module exports
    typedef ModuleInfo (*GetInfoFunc)();
    typedef int (*InitFunc)(const KernelBridge*, void*);
    typedef void (*ShutdownFunc)();
    typedef const CommandRegistration* (*RegisterCommandsFunc)();
    typedef void (*OnMessageFunc)(void*, const char*, const char*, const char*);
    typedef void (*OnTickFunc)(void*);
    
    GetInfoFunc get_info_func_;
    InitFunc init_func_;
    ShutdownFunc shutdown_func_;
    RegisterCommandsFunc register_commands_func_;
    OnMessageFunc on_message_func_;  // Optional
    OnTickFunc on_tick_func_;         // Optional
    
    std::vector<CommandRegistration> commands_;
    DiscordBot* bot_;
    
    bool load_library();
    bool load_symbols();
    void* get_symbol(const char* name);
};

// Module loader and manager
class ModuleLoader {
public:
    explicit ModuleLoader(DiscordBot* bot);
    ~ModuleLoader();

    // Load a compiled module from path
    bool load_module(const std::string& path);
    
    // Unload a module by name
    bool unload_module(const std::string& name);
    
    // Reload a module (unload then load)
    bool reload_module(const std::string& name);
    
    // Load all modules from a directory
    size_t load_modules_from_directory(const std::string& dir);
    
    // Get list of loaded modules
    std::vector<std::string> get_loaded_modules() const;
    
    // Get module info by name
    ModuleInfo* get_module_info(const std::string& name);
    
    // Get commands from a specific module
    std::vector<std::string> get_module_commands(const std::string& name) const;
    std::vector<std::pair<std::string, std::string>>
    get_command_definitions() const;
    
    // Check if module is loaded
    bool is_module_loaded(const std::string& name) const;
    
    // Dispatch command to module
    bool dispatch_command(const std::string& command, const std::string& channel_id,
                         const std::string& user_id, const std::string& args);
    
    // Notify modules of message (for raw message handlers)
    void on_message(const std::string& channel_id, const std::string& user_id,
                   const std::string& content);
    
    // Periodic tick for modules
    void on_tick();
    
    // Get kernel bridge instance
    const KernelBridge* get_bridge() const { return &bridge_; }

private:
    DiscordBot* bot_;
    std::unordered_map<std::string, std::unique_ptr<LoadedModule>> modules_;
    mutable std::recursive_mutex mutex_;
    KernelBridge bridge_;
    
    void setup_bridge();
    
    // Static bridge functions (these forward to instance methods)
    static void bridge_send_message(void* bot_context, const char* channel_id, const char* content);
    static void bridge_log(const char* level, const char* message);
    static uint64_t bridge_get_uptime(void* bot_context);
    static const char* bridge_get_guild_id(void* bot_context, const char* channel_id);
    static void* bridge_get_extension_function(void* bot_context, const char* function_name);
    static const char* bridge_get_user_roles(void* bot_context, const char* channel_id,
                                             const char* user_id);
    static int bridge_is_guild_admin(void* bot_context, const char* channel_id,
                                     const char* user_id);
};

} // namespace discord

#endif // MODULE_LOADER_HPP
