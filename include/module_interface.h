#ifndef MODULE_INTERFACE_H
#define MODULE_INTERFACE_H

/*
 * Routine Bot - Module Interface
 * 
 * This is the C-compatible interface for creating bot modules.
 * Supports: Assembly, C, C++, FORTRAN, and Lua modules
 * 
 * All modules must implement the functions marked as REQUIRED.
 * Compiled modules (C/C++/Assembly/FORTRAN) are loaded as shared libraries (.dll/.so)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// Module API version - increment when breaking changes occur
#define MODULE_API_VERSION 4

// Module types
typedef enum {
    MODULE_TYPE_NATIVE = 0,  // C/C++/Assembly compiled module
    MODULE_TYPE_LUA = 1      // Lua script module
} ModuleType;

// Module information structure
typedef struct {
    const char* name;          // Module name (e.g., "admin_tools")
    const char* version;       // Module version (e.g., "1.0.0")
    const char* author;        // Module author
    const char* description;   // Short description
    uint32_t api_version;      // API version this module was built for
    ModuleType type;           // Module type
} ModuleInfo;

// Command callback function type
// Parameters:
//   bot_context: Opaque pointer to bot instance (passed back to kernel)
//   channel_id: Discord channel ID where command was sent
//   user_id: Discord user ID who sent the command
//   args: Command arguments as null-terminated string
typedef void (*CommandCallback)(void* bot_context, const char* channel_id, 
                                const char* user_id, const char* args);

// Command registration structure
typedef struct {
    const char* name;           // Command name (without prefix)
    const char* description;    // Command description for help
    CommandCallback callback;   // Function to call when command is invoked
} CommandRegistration;

// Kernel bridge functions (provided by the bot kernel to modules)
typedef struct {
    // Send a message to a Discord channel
    void (*send_message)(void* bot_context, const char* channel_id, const char* content);
    
    // Log a message (for debugging)
    void (*log)(const char* level, const char* message);
    
    // Get bot uptime in seconds
    uint64_t (*get_uptime)(void* bot_context);

    // Resolve the Discord guild that owns a channel.
    // Returns NULL for direct messages or when the guild is unknown.
    const char* (*get_guild_id)(void* bot_context, const char* channel_id);

    // Query a function exported by a loaded kernel extension.
    // Returns NULL when no extension provides the requested function.
    void* (*get_extension_function)(void* bot_context, const char* function_name);

    // Get a comma-separated list of Discord role IDs cached from the current
    // guild message. Returns an empty string when no roles are available.
    const char* (*get_user_roles)(void* bot_context, const char* channel_id,
                                  const char* user_id);

    // Return non-zero when the user owns the guild or has Administrator or
    // Manage Guild through one of the roles cached from Discord.
    int (*is_guild_admin)(void* bot_context, const char* channel_id,
                          const char* user_id);
    
    // Reserved for future expansion
    void* reserved[4];
} KernelBridge;

// === REQUIRED EXPORTS ===
// All modules MUST export these functions

// Get module information
// REQUIRED: This function must be exported by all modules
ModuleInfo module_get_info(void);

// Initialize the module
// REQUIRED: Called when module is loaded
// Returns: 0 on success, non-zero on failure
int module_init(const KernelBridge* bridge, void* bot_context);

// Shutdown the module
// REQUIRED: Called when module is unloaded or bot is shutting down
void module_shutdown(void);

// Register commands with the kernel
// REQUIRED: Return array of commands, terminated by entry with NULL name
// The returned array must remain valid until module_shutdown() is called
const CommandRegistration* module_register_commands(void);

// === OPTIONAL EXPORTS ===

// Called when a message is received (if module wants to handle raw messages)
// OPTIONAL: Only export if you need raw message handling
void module_on_message(void* bot_context, const char* channel_id, 
                       const char* user_id, const char* content);

// Called periodically (every ~1 second)
// OPTIONAL: Only export if you need periodic tasks
void module_on_tick(void* bot_context);

#ifdef __cplusplus
}
#endif

#endif // MODULE_INTERFACE_H
