#include "lua_module.hpp"
#include "discord_bot.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace discord {

// === LuaModule Implementation ===

LuaModule::LuaModule(const std::string& path, const std::string& name)
    : path_(path), name_(name), loaded_(false), L_(nullptr), bot_(nullptr) {}

LuaModule::~LuaModule() {
    if (L_) unload();
}

bool LuaModule::load(DiscordBot* bot) {
    bot_ = bot;
    
    if (!init_lua()) {
        std::cerr << "Failed to initialize Lua state" << std::endl;
        return false;
    }
    
    setup_bridge();
    
    if (!load_script()) {
        std::cerr << "Failed to load Lua script: " << path_ << std::endl;
        unload();
        return false;
    }
    
    read_module_info();
    
    // Get registered commands
    lua_getglobal(L_, "commands");
    if (lua_istable(L_, -1)) {
        lua_pushnil(L_);
        while (lua_next(L_, -2) != 0) {
            if (lua_isstring(L_, -2)) {
                commands_.push_back(lua_tostring(L_, -2));
                std::cout << "  Registered Lua command: ~" << lua_tostring(L_, -2) << std::endl;
            }
            lua_pop(L_, 1);
        }
    }
    lua_pop(L_, 1);
    
    // Call on_load if present
    lua_getglobal(L_, "on_load");
    if (lua_isfunction(L_, -1)) {
        if (lua_pcall(L_, 0, 0, 0) != LUA_OK) {
            std::cerr << "Error in on_load: " << lua_tostring(L_, -1) << std::endl;
            lua_pop(L_, 1);
            unload();
            return false;
        }
    } else {
        lua_pop(L_, 1);
    }
    
    loaded_ = true;
    std::cout << "Loaded Lua module: " << name_ << " v" << version_ << std::endl;
    return true;
}

void LuaModule::unload() {
    if (!L_) return;
    
    // Call on_unload if present
    if (loaded_) {
        lua_getglobal(L_, "on_unload");
        if (lua_isfunction(L_, -1)) {
            if (lua_pcall(L_, 0, 0, 0) != LUA_OK) {
                std::cerr << "Error in on_unload: " << lua_tostring(L_, -1) << std::endl;
                lua_pop(L_, 1);
            }
        } else {
            lua_pop(L_, 1);
        }
    }
    lua_close(L_);
    L_ = nullptr;
    
    loaded_ = false;
    std::cout << "Unloaded Lua module: " << name_ << std::endl;
}

bool LuaModule::dispatch_command(const std::string& command, const std::string& channel_id,
                                const std::string& user_id, const std::string& args) {
    if (!loaded_) return false;
    
    lua_getglobal(L_, "commands");
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }
    
    lua_getfield(L_, -1, command.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 2);
        return false;
    }
    
    // Push arguments
    lua_pushstring(L_, channel_id.c_str());
    lua_pushstring(L_, user_id.c_str());
    lua_pushstring(L_, args.c_str());
    
    if (lua_pcall(L_, 3, 0, 0) != LUA_OK) {
        std::cerr << "Error executing Lua command '" << command << "': "
                  << lua_tostring(L_, -1) << std::endl;
        lua_pop(L_, 1);
        lua_pop(L_, 1); // Pop commands table
        return false;
    }
    
    lua_pop(L_, 1); // Pop commands table
    return true;
}

void LuaModule::on_message(const std::string& channel_id, const std::string& user_id,
                          const std::string& content) {
    if (!loaded_) return;
    
    lua_getglobal(L_, "on_message");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return;
    }
    
    lua_pushstring(L_, channel_id.c_str());
    lua_pushstring(L_, user_id.c_str());
    lua_pushstring(L_, content.c_str());
    
    if (lua_pcall(L_, 3, 0, 0) != LUA_OK) {
        std::cerr << "Error in on_message: " << lua_tostring(L_, -1) << std::endl;
        lua_pop(L_, 1);
    }
}

void LuaModule::on_tick() {
    if (!loaded_) return;
    
    lua_getglobal(L_, "on_tick");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return;
    }
    
    if (lua_pcall(L_, 0, 0, 0) != LUA_OK) {
        std::cerr << "Error in on_tick: " << lua_tostring(L_, -1) << std::endl;
        lua_pop(L_, 1);
    }
}

bool LuaModule::init_lua() {
    L_ = luaL_newstate();
    if (!L_) {
        return false;
    }
    
    luaL_openlibs(L_);
    return true;
}

void LuaModule::setup_bridge() {
    // Store bot pointer in Lua registry
    lua_pushlightuserdata(L_, (void*)bot_);
    lua_setglobal(L_, "__bot_context");
    
    // Create bot table with bridge functions
    lua_newtable(L_);
    
    lua_pushcfunction(L_, lua_send_message);
    lua_setfield(L_, -2, "send_message");
    
    lua_pushcfunction(L_, lua_log);
    lua_setfield(L_, -2, "log");
    
    lua_pushcfunction(L_, lua_get_uptime);
    lua_setfield(L_, -2, "get_uptime");
    
    lua_setglobal(L_, "bot");
}

bool LuaModule::load_script() {
    if (luaL_dofile(L_, path_.c_str()) != LUA_OK) {
        std::cerr << "Lua error: " << lua_tostring(L_, -1) << std::endl;
        lua_pop(L_, 1);
        return false;
    }
    return true;
}

void LuaModule::read_module_info() {
    // Read module_info table
    lua_getglobal(L_, "module_info");
    if (lua_istable(L_, -1)) {
        lua_getfield(L_, -1, "version");
        if (lua_isstring(L_, -1)) {
            version_ = lua_tostring(L_, -1);
        }
        lua_pop(L_, 1);
        
        lua_getfield(L_, -1, "author");
        if (lua_isstring(L_, -1)) {
            author_ = lua_tostring(L_, -1);
        }
        lua_pop(L_, 1);
    }
    lua_pop(L_, 1);
    
    if (version_.empty()) version_ = "1.0.0";
    if (author_.empty()) author_ = "Unknown";
}

// === Lua Bridge Functions ===

int LuaModule::lua_send_message(lua_State* L) {
    // Get bot context
    lua_getglobal(L, "__bot_context");
    DiscordBot* bot = static_cast<DiscordBot*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    
    if (!bot) {
        return luaL_error(L, "Invalid bot context");
    }
    
    const char* channel_id = luaL_checkstring(L, 1);
    const char* content = luaL_checkstring(L, 2);
    
    bot->send_message(channel_id, content);
    return 0;
}

int LuaModule::lua_log(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    std::cout << "[LUA] " << message << std::endl;
    return 0;
}

int LuaModule::lua_get_uptime(lua_State* L) {
    lua_getglobal(L, "__bot_context");
    DiscordBot* bot = static_cast<DiscordBot*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    lua_pushinteger(L, bot ? static_cast<lua_Integer>(bot->get_uptime()) : 0);
    return 1;
}

// === LuaModuleLoader Implementation ===

LuaModuleLoader::LuaModuleLoader(DiscordBot* bot) : bot_(bot) {}

LuaModuleLoader::~LuaModuleLoader() {
    modules_.clear();
}

bool LuaModuleLoader::load_module(const std::string& path) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    fs::path p(path);
    std::string name = p.stem().string();
    
    if (is_module_loaded(name)) {
        std::cerr << "Lua module already loaded: " << name << std::endl;
        return false;
    }
    
    auto module = std::make_unique<LuaModule>(path, name);
    if (!module->load(bot_)) {
        return false;
    }
    
    modules_[name] = std::move(module);
    return true;
}

bool LuaModuleLoader::unload_module(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return false;
    }
    
    modules_.erase(it);
    return true;
}

bool LuaModuleLoader::reload_module(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return false;
    }
    
    std::string path = it->second->path_;
    
    if (!unload_module(name)) {
        return false;
    }
    
    return load_module(path);
}

size_t LuaModuleLoader::load_modules_from_directory(const std::string& dir) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    size_t loaded_count = 0;
    
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        // Silently return 0 - caller will try other paths
        return 0;
    }
    
    std::cout << "Scanning for Lua modules in: " << dir << std::endl;
    
    std::error_code ec;
    fs::directory_iterator iterator(dir, ec);
    if (ec) {
        std::cerr << "Cannot scan Lua module directory " << dir << ": "
                  << ec.message() << std::endl;
        return 0;
    }
    for (const auto& entry : iterator) {
        if (!entry.is_regular_file(ec) || ec) {
            ec.clear();
            continue;
        }
        
        std::string ext = entry.path().extension().string();
        if (ext == ".lua") {
            std::cout << "Found Lua module: " << entry.path().filename().string() << std::endl;
            if (load_module(entry.path().string())) {
                loaded_count++;
            }
        }
    }
    
    return loaded_count;
}

std::vector<std::string> LuaModuleLoader::get_loaded_modules() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& pair : modules_) {
        names.push_back(pair.first);
    }
    return names;
}

std::vector<std::string> LuaModuleLoader::get_module_commands(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return {};
    }
    return it->second->get_commands();
}

bool LuaModuleLoader::is_module_loaded(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return modules_.find(name) != modules_.end();
}

bool LuaModuleLoader::dispatch_command(const std::string& command, const std::string& channel_id,
                                      const std::string& user_id, const std::string& args) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::cout << "[LUA] Searching " << modules_.size() << " module(s) for command: " << command << std::endl;
    
    for (const auto& pair : modules_) {
        const auto& module = pair.second;
        auto cmds = module->get_commands();
        
        std::cout << "[LUA] Module '" << pair.first << "' has " << cmds.size() << " command(s)" << std::endl;
        
        for (const auto& cmd : cmds) {
            std::cout << "[LUA]   - " << cmd << std::endl;
            if (cmd == command) {
                std::cout << "[LUA] Dispatching to module: " << pair.first << std::endl;
                return module->dispatch_command(command, channel_id, user_id, args);
            }
        }
    }
    
    std::cout << "[LUA] Command not found in any Lua module" << std::endl;
    return false;
}

void LuaModuleLoader::on_message(const std::string& channel_id, const std::string& user_id,
                                const std::string& content) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const auto& pair : modules_) {
        pair.second->on_message(channel_id, user_id, content);
    }
}

void LuaModuleLoader::on_tick() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const auto& pair : modules_) {
        pair.second->on_tick();
    }
}

} // namespace discord
