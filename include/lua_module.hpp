#ifndef LUA_MODULE_HPP
#define LUA_MODULE_HPP

#include "discord_bot.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <vector>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace discord {

class DiscordBot;

// Represents a Lua script module
class LuaModule {
    friend class LuaModuleLoader;  // Allow LuaModuleLoader to access private members
    
public:
    LuaModule(const std::string& path, const std::string& name);
    ~LuaModule();

    bool load(DiscordBot* bot);
    void unload();
    
    bool is_loaded() const { return loaded_; }
    std::string get_name() const { return name_; }
    std::string get_version() const { return version_; }
    std::string get_author() const { return author_; }
    
    std::vector<std::string> get_commands() const { return commands_; }
    
    // Dispatch command to Lua handler
    bool dispatch_command(const std::string& command, const std::string& channel_id,
                         const std::string& user_id, const std::string& args);
    
    // Call on_message handler if present
    void on_message(const std::string& channel_id, const std::string& user_id,
                   const std::string& content);
    
    // Call on_tick handler if present
    void on_tick();

private:
    std::string path_;
    std::string name_;
    std::string version_;
    std::string author_;
    bool loaded_;
    
    lua_State* L_;
    DiscordBot* bot_;
    std::vector<std::string> commands_;
    
    bool init_lua();
    void setup_bridge();
    bool load_script();
    void read_module_info();
    
    // Bridge functions (C++ -> Lua)
    static int lua_send_message(lua_State* L);
    static int lua_log(lua_State* L);
    static int lua_get_uptime(lua_State* L);
};

// Lua module manager
class LuaModuleLoader {
public:
    explicit LuaModuleLoader(DiscordBot* bot);
    ~LuaModuleLoader();

    // Load a Lua module from path
    bool load_module(const std::string& path);
    
    // Unload a module by name
    bool unload_module(const std::string& name);
    
    // Reload a module
    bool reload_module(const std::string& name);
    
    // Load all Lua modules from directory
    size_t load_modules_from_directory(const std::string& dir);
    
    // Get list of loaded modules
    std::vector<std::string> get_loaded_modules() const;
    
    // Get commands from a specific module
    std::vector<std::string> get_module_commands(const std::string& name) const;
    
    // Check if module is loaded
    bool is_module_loaded(const std::string& name) const;
    
    // Dispatch command to appropriate module
    bool dispatch_command(const std::string& command, const std::string& channel_id,
                         const std::string& user_id, const std::string& args);
    
    // Notify modules of message
    void on_message(const std::string& channel_id, const std::string& user_id,
                   const std::string& content);
    
    // Periodic tick
    void on_tick();

private:
    DiscordBot* bot_;
    std::unordered_map<std::string, std::unique_ptr<LuaModule>> modules_;
    mutable std::recursive_mutex mutex_;
};

} // namespace discord

#endif // LUA_MODULE_HPP
