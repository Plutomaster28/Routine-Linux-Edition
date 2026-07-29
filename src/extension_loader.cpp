#include "extension_loader.hpp"
#include "discord_bot.hpp"
#include <iostream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

namespace discord {

// Static pointer for kernel bridge callbacks
static DiscordBot* g_bot_instance = nullptr;

// ============================================================================
// LoadedExtension Implementation
// ============================================================================

void LoadedExtension::unload() {
    if (handle) {
        // Call shutdown if available
        if (initialized && shutdown) {
            shutdown();
        }
        
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        handle = nullptr;
    }
    
    info = nullptr;
    api = nullptr;
    get_info = nullptr;
    init = nullptr;
    shutdown = nullptr;
    get_api = nullptr;
    initialized = false;
}

bool LoadedExtension::has_capability(uint32_t capability) const {
    if (!info) return false;
    
    // Check if capability is in the flags
    if (info->capabilities & capability) {
        // If extension API has runtime check, use it
        if (api && api->has_capability) {
            return api->has_capability(capability) != 0;
        }
        return true;
    }
    
    return false;
}

void* LoadedExtension::get_function(const char* function_name) const {
    if (!api || !api->get_function) return nullptr;
    return api->get_function(function_name);
}

// ============================================================================
// ExtensionLoader Implementation
// ===================================================================

ExtensionLoader::ExtensionLoader(discord::DiscordBot* bot)
    : bot_(bot) {
    g_bot_instance = bot;
    setup_kernel_bridge();
}

ExtensionLoader::~ExtensionLoader() {
    unload_all();
}

void ExtensionLoader::setup_kernel_bridge() {
    std::memset(&kernel_bridge_, 0, sizeof(kernel_bridge_));
    
    kernel_bridge_.log = kernel_log;
    kernel_bridge_.get_uptime = kernel_get_uptime;
    kernel_bridge_.allocate = kernel_allocate;
    kernel_bridge_.deallocate = kernel_deallocate;
}

bool ExtensionLoader::load_extension(const std::string& path) {
    std::cout << "[EXTENSION] Loading: " << path << std::endl;
    
    auto ext = std::make_unique<LoadedExtension>();
    ext->path = path;
    
    // Load the library
#ifdef _WIN32
    ext->handle = LoadLibraryA(path.c_str());
    if (!ext->handle) {
        std::cerr << "[EXTENSION] Failed to load: " << path 
                  << " (Error: " << GetLastError() << ")" << std::endl;
        return false;
    }
#else
    ext->handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!ext->handle) {
        std::cerr << "[EXTENSION] Failed to load: " << path 
                  << " (" << dlerror() << ")" << std::endl;
        return false;
    }
#endif
    
    // Load required functions
#ifdef _WIN32
    ext->get_info = (extension_get_info_func)GetProcAddress(ext->handle, EXTENSION_GET_INFO_NAME);
    ext->init = (extension_init_func)GetProcAddress(ext->handle, EXTENSION_INIT_NAME);
    ext->shutdown = (extension_shutdown_func)GetProcAddress(ext->handle, EXTENSION_SHUTDOWN_NAME);
    ext->get_api = (extension_get_api_func)GetProcAddress(ext->handle, EXTENSION_GET_API_NAME);
#else
    ext->get_info = (extension_get_info_func)dlsym(ext->handle, EXTENSION_GET_INFO_NAME);
    ext->init = (extension_init_func)dlsym(ext->handle, EXTENSION_INIT_NAME);
    ext->shutdown = (extension_shutdown_func)dlsym(ext->handle, EXTENSION_SHUTDOWN_NAME);
    ext->get_api = (extension_get_api_func)dlsym(ext->handle, EXTENSION_GET_API_NAME);
#endif
    
    if (!ext->get_info || !ext->init || !ext->get_api) {
        std::cerr << "[EXTENSION] Missing required exports in: " << path << std::endl;
        return false;
    }
    
    // Get extension info
    ext->info = ext->get_info();
    if (!ext->info) {
        std::cerr << "[EXTENSION] Failed to get extension info: " << path << std::endl;
        return false;
    }
    if (!ext->info->name || !*ext->info->name || !ext->info->description ||
        !ext->info->version || !ext->info->author) {
        std::cerr << "[EXTENSION] Invalid metadata in: " << path << std::endl;
        return false;
    }
    
    // Check API version
    if (ext->info->api_version != EXTENSION_API_VERSION) {
        std::cerr << "[EXTENSION] Incompatible API version in: " << path 
                  << " (expected " << EXTENSION_API_VERSION 
                  << ", got " << ext->info->api_version << ")" << std::endl;
        return false;
    }
    
    ext->name = ext->info->name;
    if (get_extension(ext->name)) {
        std::cerr << "[EXTENSION] Duplicate extension name: "
                  << ext->name << std::endl;
        return false;
    }
    
    // Initialize extension
    int init_result = ext->init(&kernel_bridge_);
    if (init_result != 0) {
        std::cerr << "[EXTENSION] Initialization failed for: " << ext->name 
                  << " (code: " << init_result << ")" << std::endl;
        return false;
    }
    ext->initialized = true;
    
    // Get API table
    ext->api = ext->get_api();
    if (!ext->api || !ext->api->get_function) {
        std::cerr << "[EXTENSION] Invalid API table for: " << ext->name
                  << std::endl;
        return false;
    }
    
    std::cout << "[EXTENSION] ✓ Loaded: " << ext->name 
              << " v" << ext->info->version 
              << " (" << ext->info->description << ")" << std::endl;
    
    // Show capabilities
    std::cout << "[EXTENSION]   Capabilities: 0x" << std::hex 
              << ext->info->capabilities << std::dec << std::endl;
    
    extensions_.push_back(std::move(ext));
    return true;
}

size_t ExtensionLoader::load_extensions_from_directory(const std::string& directory) {
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        return 0;
    }
    
    size_t loaded = 0;
    
    std::error_code ec;
    fs::directory_iterator iterator(directory, ec);
    if (ec) {
        std::cerr << "[EXTENSION] Cannot scan " << directory << ": "
                  << ec.message() << std::endl;
        return 0;
    }
    for (const auto& entry : iterator) {
        if (!entry.is_regular_file(ec) || ec) {
            ec.clear();
            continue;
        }
        
        std::string path = entry.path().string();
        std::string ext = entry.path().extension().string();
        
        // Load .dll on Windows, .so on Linux
#ifdef _WIN32
        if (ext != ".dll") continue;
#else
        if (ext != ".so") continue;
#endif
        
        if (load_extension(path)) {
            loaded++;
        }
    }
    
    std::cout << "[EXTENSION] Loaded " << loaded << " extension(s) from: " 
              << directory << std::endl;
    
    return loaded;
}

bool ExtensionLoader::unload_extension(const std::string& name) {
    for (auto it = extensions_.begin(); it != extensions_.end(); ++it) {
        if ((*it)->name == name) {
            std::cout << "[EXTENSION] Unloading: " << name << std::endl;
            extensions_.erase(it);
            return true;
        }
    }
    return false;
}

void ExtensionLoader::unload_all() {
    std::cout << "[EXTENSION] Unloading all extensions..." << std::endl;
    // Extensions may depend on capabilities loaded before them. Destroy in
    // strict reverse initialization order instead of relying on a vector
    // implementation's element-destruction order.
    while (!extensions_.empty()) {
        extensions_.pop_back();
    }
}

bool ExtensionLoader::reload_all() {
    std::cout << "[EXTENSION] Reloading all extensions..." << std::endl;
    
    // Save paths
    std::vector<std::string> paths;
    for (const auto& ext : extensions_) {
        paths.push_back(ext->path);
    }
    
    // Unload all
    unload_all();
    
    // Reload all
    bool all_loaded = true;
    for (const auto& path : paths) {
        if (!load_extension(path)) {
            all_loaded = false;
        }
    }
    
    return all_loaded;
}

LoadedExtension* ExtensionLoader::get_extension(const std::string& name) {
    for (auto& ext : extensions_) {
        if (ext->name == name) {
            return ext.get();
        }
    }
    return nullptr;
}

LoadedExtension* ExtensionLoader::find_extension_with_capability(uint32_t capability) {
    for (auto& ext : extensions_) {
        if (ext->has_capability(capability)) {
            return ext.get();
        }
    }
    return nullptr;
}

std::vector<LoadedExtension*> ExtensionLoader::get_extensions_by_type(ExtensionType type) {
    std::vector<LoadedExtension*> result;
    
    for (auto& ext : extensions_) {
        if (ext->info && ext->info->type == type) {
            result.push_back(ext.get());
        }
    }
    
    return result;
}

void* ExtensionLoader::get_function(const char* function_name) {
    for (auto& ext : extensions_) {
        void* func = ext->get_function(function_name);
        if (func) {
            return func;
        }
    }
    return nullptr;
}

// ============================================================================
// Kernel Bridge Implementation
// ============================================================================

void ExtensionLoader::kernel_log(const char* message) {
    std::cout << "[KERNEL->EXT] " << message << std::endl;
}

uint64_t ExtensionLoader::kernel_get_uptime() {
    if (g_bot_instance) {
        return g_bot_instance->get_uptime();
    }
    return 0;
}

void* ExtensionLoader::kernel_allocate(size_t size) {
    return malloc(size);
}

void ExtensionLoader::kernel_deallocate(void* ptr) {
    free(ptr);
}

} // namespace discord
