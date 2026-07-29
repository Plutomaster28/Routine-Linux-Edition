/*
 * Example C Module for Routine Bot
 * 
 * This demonstrates how to create a compiled module in C.
 * Compile as a shared library (.dll on Windows, .so on Linux)
 * 
 * Windows (MSVC):
 *   cl /LD /I..\..\include example_c_module.c /Fe:example.dll
 * 
 * Windows (MinGW/MSYS2):
 *   gcc -shared -I../../include example_c_module.c -o example.dll
 * 
 * Linux:
 *   gcc -shared -fPIC -I../../include example_c_module.c -o example.so
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "module_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Module-level state
static const KernelBridge* g_bridge = NULL;
static void* g_bot_context = NULL;

// === Helper Functions ===

static void send_reply(const char* channel_id, const char* message) {
    if (g_bridge && g_bridge->send_message) {
        g_bridge->send_message(g_bot_context, channel_id, message);
    }
}

static void log_message(const char* message) {
    if (g_bridge && g_bridge->log) {
        g_bridge->log("INFO", message);
    }
}

// === Command Implementations ===

static void cmd_hello(void* bot_context, const char* channel_id, 
                      const char* user_id, const char* args) {
    (void)bot_context;
    (void)args;
    char buffer[512];
    snprintf(buffer, sizeof(buffer), 
             "Hello from C! \n"
             "**User ID:** %s\n"
             "*This is a native compiled module!*",
             user_id);
    send_reply(channel_id, buffer);
}

static void cmd_timestamp(void* bot_context, const char* channel_id,
                         const char* user_id, const char* args) {
    (void)bot_context;
    (void)user_id;
    (void)args;
    time_t now = time(NULL);
    struct tm time_parts;
#ifdef _WIN32
    localtime_s(&time_parts, &now);
#else
    localtime_r(&now, &time_parts);
#endif
    
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &time_parts);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "**Current Time:** %s\n"
             "*Timestamp: %ld*",
             time_str, (long)now);
    send_reply(channel_id, buffer);
}

static void cmd_reverse(void* bot_context, const char* channel_id,
                       const char* user_id, const char* args) {
    (void)bot_context;
    (void)user_id;
    if (!args || strlen(args) == 0) {
        send_reply(channel_id, "Usage: ~reverse <text>");
        return;
    }
    
    size_t len = strlen(args);
    char* reversed = (char*)malloc(len + 1);
    if (!reversed) {
        send_reply(channel_id, "Unable to allocate a response buffer.");
        return;
    }
    
    for (size_t i = 0; i < len; i++) {
        reversed[i] = args[len - 1 - i];
    }
    reversed[len] = '\0';
    
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "**Reversed:** %s",
             reversed);
    
    free(reversed);
    send_reply(channel_id, buffer);
}

// === Module Interface Implementation ===

// Command registration table
static CommandRegistration commands[] = {
    { "hello", "Say hello from C module", cmd_hello },
    { "timestamp", "Get current timestamp", cmd_timestamp },
    { "reverse", "Reverse a string", cmd_reverse },
    { NULL, NULL, NULL }  // Terminator
};

ModuleInfo module_get_info(void) {
    ModuleInfo info;
    info.name = "example_c";
    info.version = "1.0.0";
    info.author = "Routine Team";
    info.description = "Example C module demonstrating native compilation";
    info.api_version = MODULE_API_VERSION;
    info.type = MODULE_TYPE_NATIVE;
    return info;
}

int module_init(const KernelBridge* bridge, void* bot_context) {
    g_bridge = bridge;
    g_bot_context = bot_context;
    
    log_message("C module initialized successfully!");
    return 0;
}

void module_shutdown(void) {
    log_message("C module shutting down...");
    g_bridge = NULL;
    g_bot_context = NULL;
}

const CommandRegistration* module_register_commands(void) {
    return commands;
}

// Optional: Handle raw messages
void module_on_message(void* bot_context, const char* channel_id,
                      const char* user_id, const char* content) {
    (void)bot_context;
    (void)channel_id;
    (void)user_id;
    // Example: Log messages containing "test"
    if (strstr(content, "test") != NULL) {
        log_message("Detected 'test' in message");
    }
}

// Optional: Periodic tick
void module_on_tick(void* bot_context) {
    (void)bot_context;
    // Example: Could be used for periodic tasks
    // Be careful not to do heavy work here
}
