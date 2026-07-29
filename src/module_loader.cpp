#include "module_loader.hpp"
#include "discord_bot.hpp"
#include "extension_loader.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <cstring>

namespace fs = std::filesystem;

namespace discord {

// === LoadedModule Implementation ===

LoadedModule::LoadedModule(const std::string& path, const std::string& name)
    : path_(path), name_(name), loaded_(false), handle_(nullptr),
      get_info_func_(nullptr), init_func_(nullptr), shutdown_func_(nullptr),
      register_commands_func_(nullptr), on_message_func_(nullptr),
      on_tick_func_(nullptr), bot_(nullptr) {
    memset(&info_, 0, sizeof(info_));
}

LoadedModule::~LoadedModule() {
    if (loaded_) {
        unload();
    }
}

bool LoadedModule::load(DiscordBot* bot) {
    bot_ = bot;
    
    if (!load_library()) {
        std::cerr << "Failed to load library: " << path_ << std::endl;
        return false;
    }
    
    if (!load_symbols()) {
        std::cerr << "Failed to load symbols from: " << path_ << std::endl;
        unload();
        return false;
    }
    
    // Get module info
    info_ = get_info_func_();
    if (!info_.name || !*info_.name || !info_.version || !info_.author ||
        !info_.description) {
        std::cerr << "Module returned invalid metadata: " << path_ << std::endl;
        unload();
        return false;
    }
    
    // Verify API version
    if (info_.api_version != MODULE_API_VERSION) {
        std::cerr << "Module API version mismatch: expected " << MODULE_API_VERSION
                  << ", got " << info_.api_version << std::endl;
        unload();
        return false;
    }
    
    std::cout << "Loaded module: " << info_.name << " v" << info_.version
              << " by " << info_.author << std::endl;
    
    loaded_ = true;
    return true;
}

void LoadedModule::unload() {
    if (!loaded_) return;
    
    if (shutdown_func_) {
        shutdown_func_();
    }
    
    if (handle_) {
#ifdef _WIN32
        FreeLibrary(handle_);
#else
        dlclose(handle_);
#endif
        handle_ = nullptr;
    }
    
    loaded_ = false;
    std::cout << "Unloaded module: " << name_ << std::endl;
}

std::vector<std::string> LoadedModule::get_commands() const {
    std::vector<std::string> cmd_names;
    for (const auto& cmd : commands_) {
        if (cmd.name) {
            cmd_names.push_back(cmd.name);
        }
    }
    return cmd_names;
}

std::vector<std::pair<std::string, std::string>>
LoadedModule::get_command_definitions() const {
    std::vector<std::pair<std::string, std::string>> definitions;
    for (const auto& command : commands_) {
        if (command.name && command.description) {
            definitions.emplace_back(command.name, command.description);
        }
    }
    return definitions;
}

bool LoadedModule::load_library() {
#ifdef _WIN32
    handle_ = LoadLibraryA(path_.c_str());
    if (!handle_) {
        DWORD error = GetLastError();
        std::cerr << "LoadLibrary failed with error: " << error << std::endl;
    }
    return handle_ != nullptr;
#else
    handle_ = dlopen(path_.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle_) {
        std::cerr << "dlopen failed: " << dlerror() << std::endl;
    }
    return handle_ != nullptr;
#endif
}

bool LoadedModule::load_symbols() {
    // Load required symbols
    get_info_func_ = (GetInfoFunc)get_symbol("module_get_info");
    init_func_ = (InitFunc)get_symbol("module_init");
    shutdown_func_ = (ShutdownFunc)get_symbol("module_shutdown");
    register_commands_func_ = (RegisterCommandsFunc)get_symbol("module_register_commands");
    
    if (!get_info_func_ || !init_func_ || !shutdown_func_ || !register_commands_func_) {
        std::cerr << "Missing required module exports" << std::endl;
        return false;
    }
    
    // Load optional symbols
    on_message_func_ = (OnMessageFunc)get_symbol("module_on_message");
    on_tick_func_ = (OnTickFunc)get_symbol("module_on_tick");
    
    return true;
}

void* LoadedModule::get_symbol(const char* name) {
#ifdef _WIN32
    return (void*)GetProcAddress(handle_, name);
#else
    return dlsym(handle_, name);
#endif
}

// === ModuleLoader Implementation ===

ModuleLoader::ModuleLoader(DiscordBot* bot) : bot_(bot) {
    setup_bridge();
}

ModuleLoader::~ModuleLoader() {
    // Unload all modules
    modules_.clear();
}

void ModuleLoader::setup_bridge() {
    memset(&bridge_, 0, sizeof(bridge_));
    bridge_.send_message = bridge_send_message;
    bridge_.log = bridge_log;
    bridge_.get_uptime = bridge_get_uptime;
    bridge_.get_guild_id = bridge_get_guild_id;
    bridge_.get_extension_function = bridge_get_extension_function;
    bridge_.get_user_roles = bridge_get_user_roles;
    bridge_.is_guild_admin = bridge_is_guild_admin;
}

bool ModuleLoader::load_module(const std::string& path) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // Extract module name from path
    fs::path p(path);
    std::string name = p.stem().string();
    
    // Check if already loaded
    if (is_module_loaded(name)) {
        std::cerr << "Module already loaded: " << name << std::endl;
        return false;
    }
    
    // Create and load module
    auto module = std::make_unique<LoadedModule>(path, name);
    if (!module->load(bot_)) {
        return false;
    }
    
    // Initialize module with bridge
    int init_result = module->init_func_(&bridge_, (void*)bot_);
    if (init_result != 0) {
        std::cerr << "Module initialization failed: " << name << std::endl;
        return false;
    }
    
    // Register commands
    const CommandRegistration* cmds = module->register_commands_func_();
    if (cmds) {
        size_t i = 0;
        for (; i < 1024 && cmds[i].name != nullptr; ++i) {
            if (!*cmds[i].name || !cmds[i].description || !cmds[i].callback) {
                std::cerr << "Invalid command registration in module: "
                          << name << std::endl;
                return false;
            }
            module->commands_.push_back(cmds[i]);
            std::cout << "  Registered command: ~" << cmds[i].name << std::endl;
        }
        if (i == 1024) {
            std::cerr << "Unterminated or excessive command table in module: "
                      << name << std::endl;
            return false;
        }
    }
    
    modules_[name] = std::move(module);
    return true;
}

bool ModuleLoader::unload_module(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return false;
    }
    
    modules_.erase(it);
    return true;
}

bool ModuleLoader::reload_module(const std::string& name) {
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

size_t ModuleLoader::load_modules_from_directory(const std::string& dir) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    size_t loaded_count = 0;
    
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        // Silently return 0 - caller will try other paths
        return 0;
    }
    
    std::cout << "Scanning for native modules in: " << dir << std::endl;
    
    std::error_code ec;
    fs::directory_iterator iterator(dir, ec);
    if (ec) {
        std::cerr << "Cannot scan module directory " << dir << ": "
                  << ec.message() << std::endl;
        return 0;
    }
    for (const auto& entry : iterator) {
        if (!entry.is_regular_file(ec) || ec) {
            ec.clear();
            continue;
        }
        
        std::string ext = entry.path().extension().string();
        
#ifdef _WIN32
        if (ext == ".dll") {
#else
        if (ext == ".so") {
#endif
            std::cout << "Found module: " << entry.path().filename().string() << std::endl;
            if (load_module(entry.path().string())) {
                loaded_count++;
            }
        }
    }
    
    return loaded_count;
}

std::vector<std::string> ModuleLoader::get_loaded_modules() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& pair : modules_) {
        names.push_back(pair.first);
    }
    return names;
}

ModuleInfo* ModuleLoader::get_module_info(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return nullptr;
    }
    return &it->second->info_;
}

std::vector<std::string> ModuleLoader::get_module_commands(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return {};
    }
    return it->second->get_commands();
}

std::vector<std::pair<std::string, std::string>>
ModuleLoader::get_command_definitions() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::pair<std::string, std::string>> definitions;
    for (const auto& entry : modules_) {
        const auto module_definitions =
            entry.second->get_command_definitions();
        definitions.insert(definitions.end(), module_definitions.begin(),
                           module_definitions.end());
    }
    return definitions;
}

bool ModuleLoader::is_module_loaded(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return modules_.find(name) != modules_.end();
}

bool ModuleLoader::dispatch_command(const std::string& command, const std::string& channel_id,
                                   const std::string& user_id, const std::string& args) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::cout << "[NATIVE] Searching " << modules_.size() << " module(s) for command: " << command << std::endl;
    
    for (const auto& pair : modules_) {
        const auto& module = pair.second;
        std::cout << "[NATIVE] Module '" << pair.first << "' has " << module->commands_.size() << " command(s)" << std::endl;
        
        for (const auto& cmd : module->commands_) {
            if (cmd.name) {
                std::cout << "[NATIVE]   - " << cmd.name << std::endl;
                if (command == cmd.name) {
                    std::cout << "[NATIVE] Dispatching to module: " << pair.first << std::endl;
                    cmd.callback((void*)bot_, channel_id.c_str(), user_id.c_str(), args.c_str());
                    return true;
                }
            }
        }
    }
    
    std::cout << "[NATIVE] Command not found in any native module" << std::endl;
    return false;
}

void ModuleLoader::on_message(const std::string& channel_id, const std::string& user_id,
                             const std::string& content) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const auto& pair : modules_) {
        if (pair.second->on_message_func_) {
            pair.second->on_message_func_((void*)bot_, channel_id.c_str(),
                                         user_id.c_str(), content.c_str());
        }
    }
}

void ModuleLoader::on_tick() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const auto& pair : modules_) {
        if (pair.second->on_tick_func_) {
            pair.second->on_tick_func_((void*)bot_);
        }
    }
}

// === Bridge Functions ===

void ModuleLoader::bridge_send_message(void* bot_context, const char* channel_id, const char* content) {
    if (!bot_context || !channel_id || !content) return;
    
    DiscordBot* bot = static_cast<DiscordBot*>(bot_context);
    bot->send_message(channel_id, content);
}

void ModuleLoader::bridge_log(const char* level, const char* message) {
    if (!level || !message) return;
    
    std::string lvl(level);
    if (lvl == "ERROR" || lvl == "error") {
        std::cerr << "[MODULE] " << message << std::endl;
    } else {
        std::cout << "[MODULE] " << message << std::endl;
    }
}

uint64_t ModuleLoader::bridge_get_uptime(void* bot_context) {
    if (!bot_context) return 0;

    DiscordBot* bot = static_cast<DiscordBot*>(bot_context);
    return bot->get_uptime();
}

const char* ModuleLoader::bridge_get_guild_id(void* bot_context, const char* channel_id) {
    if (!bot_context || !channel_id) return nullptr;

    DiscordBot* bot = static_cast<DiscordBot*>(bot_context);
    static thread_local std::string guild_id;
    guild_id = bot->get_guild_id_for_channel(channel_id);
    return guild_id.empty() ? nullptr : guild_id.c_str();
}

void* ModuleLoader::bridge_get_extension_function(void* bot_context,
                                                  const char* function_name) {
    if (!bot_context || !function_name) return nullptr;

    DiscordBot* bot = static_cast<DiscordBot*>(bot_context);
    ExtensionLoader* loader = bot->get_extension_loader();
    return loader ? loader->get_function(function_name) : nullptr;
}

const char* ModuleLoader::bridge_get_user_roles(void* bot_context, const char* channel_id,
                                                const char* user_id) {
    if (!bot_context || !channel_id || !user_id) return "";
    DiscordBot* bot = static_cast<DiscordBot*>(bot_context);
    static thread_local std::string roles;
    roles = bot->get_user_roles(channel_id, user_id);
    return roles.c_str();
}

int ModuleLoader::bridge_is_guild_admin(void* bot_context, const char* channel_id,
                                        const char* user_id) {
    if (!bot_context || !channel_id || !user_id) return 0;
    DiscordBot* bot = static_cast<DiscordBot*>(bot_context);
    return bot->is_guild_admin(channel_id, user_id) ? 1 : 0;
}

} // namespace discord
