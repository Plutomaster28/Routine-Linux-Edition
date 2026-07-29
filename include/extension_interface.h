/**
 * Extension Interface for Routine Bot
 * 
 * Extensions provide low-level kernel capabilities and acceleration.
 * Unlike modules (which add commands), extensions extend the kernel's core functionality.
 * 
 * Examples:
 * - SIMD acceleration for media processing
 * - Custom memory allocators
 * - Hardware acceleration interfaces
 * - Audio/video codecs
 * - Cryptographic acceleration
 */

#ifndef EXTENSION_INTERFACE_H
#define EXTENSION_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define EXTENSION_API_VERSION 1

/**
 * Extension types - what kind of capability this extension provides
 */
typedef enum {
    EXT_TYPE_SIMD,          // SIMD acceleration (SSE, AVX, NEON, etc.)
    EXT_TYPE_MEDIA,         // Media processing (audio/video codecs)
    EXT_TYPE_CRYPTO,        // Cryptographic acceleration
    EXT_TYPE_MEMORY,        // Custom memory management
    EXT_TYPE_NETWORK,       // Network protocol acceleration
    EXT_TYPE_COMPUTE,       // General compute acceleration (GPU, etc.)
    EXT_TYPE_CUSTOM         // Custom extension type
} ExtensionType;

/**
 * Extension capability flags - what specific features are provided
 */
typedef uint32_t ExtensionCapability;
#define EXT_CAP_NONE   UINT32_C(0)
#define EXT_CAP_SSE    (UINT32_C(1) << 0)
#define EXT_CAP_SSE2   (UINT32_C(1) << 1)
#define EXT_CAP_SSE3   (UINT32_C(1) << 2)
#define EXT_CAP_AVX    (UINT32_C(1) << 3)
#define EXT_CAP_AVX2   (UINT32_C(1) << 4)
#define EXT_CAP_AVX512 (UINT32_C(1) << 5)
#define EXT_CAP_NEON   (UINT32_C(1) << 6)
#define EXT_CAP_AUDIO  (UINT32_C(1) << 7)
#define EXT_CAP_VIDEO  (UINT32_C(1) << 8)
#define EXT_CAP_GPU    (UINT32_C(1) << 9)
#define EXT_CAP_AES_NI (UINT32_C(1) << 10)
#define EXT_CAP_CUSTOM (UINT32_C(1) << 31)

/**
 * Extension information
 */
typedef struct {
    const char* name;           // Extension name (e.g., "simd_accel")
    const char* description;    // Human-readable description
    const char* version;        // Extension version (e.g., "1.0.0")
    const char* author;         // Extension author
    ExtensionType type;         // Extension type
    uint32_t capabilities;      // Bitfield of ExtensionCapability flags
    uint32_t api_version;       // Must be EXTENSION_API_VERSION
} ExtensionInfo;

/**
 * Extension function table - extensions can provide these functions
 * to be called by the kernel or modules
 */
typedef struct {
    // Initialize extension with kernel context
    int (*init)(void* kernel_context);
    
    // Shutdown and cleanup
    void (*shutdown)(void);
    
    // Query if a specific capability is available at runtime
    int (*has_capability)(uint32_t capability);
    
    // Get a function pointer by name (for custom extension functions)
    // Returns NULL if function doesn't exist
    void* (*get_function)(const char* function_name);
    
} ExtensionAPI;

/**
 * Kernel bridge for extensions - functions the kernel provides
 */
typedef struct {
    // Logging function
    void (*log)(const char* message);
    
    // Get kernel uptime in seconds
    uint64_t (*get_uptime)(void);
    
    // Allocate memory (kernel-managed)
    void* (*allocate)(size_t size);
    
    // Free kernel-managed memory
    void (*deallocate)(void* ptr);
    
    // Reserved for future use
    void* reserved[4];
    
} ExtensionKernelBridge;

/**
 * Required exports for all extensions
 */

// Get extension information
typedef ExtensionInfo* (*extension_get_info_func)(void);
#define EXTENSION_GET_INFO_NAME "extension_get_info"

// Initialize extension
typedef int (*extension_init_func)(const ExtensionKernelBridge* bridge);
#define EXTENSION_INIT_NAME "extension_init"

// Shutdown extension
typedef void (*extension_shutdown_func)(void);
#define EXTENSION_SHUTDOWN_NAME "extension_shutdown"

// Get extension API function table
typedef const ExtensionAPI* (*extension_get_api_func)(void);
#define EXTENSION_GET_API_NAME "extension_get_api"

#ifdef __cplusplus
}
#endif

#endif // EXTENSION_INTERFACE_H
