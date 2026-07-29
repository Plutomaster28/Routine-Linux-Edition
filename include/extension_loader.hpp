#ifndef EXTENSION_LOADER_HPP
#define EXTENSION_LOADER_HPP

#include "extension_interface.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE LibraryHandle;
#else
#include <dlfcn.h>
typedef void* LibraryHandle;
#endif

namespace discord {

class DiscordBot;

/**
 * Loaded extension instance
 */
class LoadedExtension {
public:
    std::string name;
    std::string path;
    LibraryHandle handle;
    ExtensionInfo* info;
    const ExtensionAPI* api;
    bool initialized;
    
    // Function pointers
    extension_get_info_func get_info;
    extension_init_func init;
    extension_shutdown_func shutdown;
    extension_get_api_func get_api;
    
    LoadedExtension()
        : handle(nullptr), info(nullptr), api(nullptr), initialized(false),
          get_info(nullptr), init(nullptr), shutdown(nullptr), get_api(nullptr) {}
    
    ~LoadedExtension() {
        unload();
    }
    
    void unload();
    
    // Check if extension has a specific capability
    bool has_capability(uint32_t capability) const;
    
    // Get a function by name from this extension
    void* get_function(const char* function_name) const;
    
    friend class ExtensionLoader;
};

/**
 * Extension loader - manages kernel extensions
 */
class ExtensionLoader {
public:
    explicit ExtensionLoader(DiscordBot* bot);
    ~ExtensionLoader();
    
    // Load a single extension from a file
    bool load_extension(const std::string& path);
    
    // Load all extensions from a directory
    size_t load_extensions_from_directory(const std::string& directory);
    
    // Unload a specific extension
    bool unload_extension(const std::string& name);
    
    // Unload all extensions
    void unload_all();
    
    // Reload all extensions
    bool reload_all();
    
    // Get a loaded extension by name
    LoadedExtension* get_extension(const std::string& name);
    
    // Get all loaded extensions
    const std::vector<std::unique_ptr<LoadedExtension>>& get_extensions() const {
        return extensions_;
    }
    
    // Find an extension that provides a specific capability
    LoadedExtension* find_extension_with_capability(uint32_t capability);
    
    // Get all extensions of a specific type
    std::vector<LoadedExtension*> get_extensions_by_type(ExtensionType type);
    
    // Get a function from any loaded extension by name
    void* get_function(const char* function_name);
    
private:
    DiscordBot* bot_;
    std::vector<std::unique_ptr<LoadedExtension>> extensions_;
    ExtensionKernelBridge kernel_bridge_;
    
    void setup_kernel_bridge();
    
    // Kernel bridge implementation functions
    static void kernel_log(const char* message);
    static uint64_t kernel_get_uptime();
    static void* kernel_allocate(size_t size);
    static void kernel_deallocate(void* ptr);
};

} // namespace discord

#endif // EXTENSION_LOADER_HPP
