#ifndef SCRIPT_ENGINE_HPP
#define SCRIPT_ENGINE_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>

namespace discord {

class DiscordBot;
class ModuleLoader;

// Forward declarations
struct ScriptAction;
struct ScriptCondition;
struct Script;

/**
 * Script Context - passed between actions, holds runtime state
 */
class ScriptContext {
public:
    ScriptContext();
    
    // Set/get variables in context
    void set(const std::string& key, const nlohmann::json& value);
    nlohmann::json get(const std::string& key) const;
    bool has(const std::string& key) const;
    
    // Template variable substitution: {{author.name}} -> actual value
    std::string substitute(const std::string& text) const;
    
    // Set event data (message, user info, etc.)
    void set_event_data(const nlohmann::json& event);
    
    const nlohmann::json& data() const { return data_; }
    
private:
    nlohmann::json data_;  // All context data as JSON
};

/**
 * Script Condition - when clause
 */
struct ScriptCondition {
    enum Type {
        STARTS_WITH,
        ENDS_WITH,
        CONTAINS,
        EQUALS,
        MATCHES_REGEX,
        HAS_ROLE,
        IN_CHANNEL,
        ALWAYS
    };
    
    Type type;
    std::string field;     // What to check (e.g., "content", "author.id")
    std::string value;     // Expected value
    
    bool evaluate(const ScriptContext& ctx) const;
};

/**
 * Script Action - a module invocation with args
 */
struct ScriptAction {
    std::string module;                           // Module name
    std::map<std::string, std::string> args;      // Arguments (can contain {{templates}})
    std::string store_result_as;                  // Optional: store result in context
    
    bool execute(ScriptContext& ctx, ModuleLoader* loader, DiscordBot* bot) const;
};

/**
 * Script - complete event → condition → action flow
 */
struct Script {
    std::string name;                        // Script identifier
    std::string event_type;                  // "message.create", "ready", etc.
    std::vector<ScriptCondition> conditions; // When clauses
    std::vector<ScriptAction> actions;       // Do clauses
    bool enabled;                            // Can be disabled without deleting
    
    // Metadata
    std::string author;                      // Who created it
    std::string created_at;                  // Timestamp
    
    bool should_run(const ScriptContext& ctx) const;
    bool execute(ScriptContext& ctx, ModuleLoader* loader, DiscordBot* bot) const;
};

/**
 * Script Parser - converts YAML-like text to Script objects
 */
class ScriptParser {
public:
    // Parse a script from text (YAML-like format)
    static std::unique_ptr<Script> parse(const std::string& text, std::string& error);
    
private:
    static bool parse_metadata(const nlohmann::json& doc, Script& script, std::string& error);
    static bool parse_conditions(const nlohmann::json& when, Script& script, std::string& error);
    static bool parse_actions(const nlohmann::json& actions, Script& script, std::string& error);
    
    static ScriptCondition::Type string_to_condition_type(const std::string& type);
};

/**
 * Script Engine - manages and executes scripts
 */
class ScriptEngine {
public:
    ScriptEngine(DiscordBot* bot, ModuleLoader* loader);
    ~ScriptEngine();
    
    // Load a script from text (Discord message)
    bool load_script(const std::string& text, const std::string& author, std::string& error);
    
    // Enable/disable scripts
    bool enable_script(const std::string& name);
    bool disable_script(const std::string& name);
    
    // Remove a script
    bool remove_script(const std::string& name);
    
    // List all scripts
    std::vector<std::string> list_scripts() const;
    
    // Get script details
    const Script* get_script(const std::string& name) const;
    
    // Handle an event (checks all scripts for this event type)
    void handle_event(const std::string& event_type, const nlohmann::json& event_data);
    
    // Save/load scripts from disk
    bool save_scripts(const std::string& path);
    bool load_scripts(const std::string& path);
    
private:
    DiscordBot* bot_;
    ModuleLoader* loader_;
    std::map<std::string, std::unique_ptr<Script>> scripts_;
    mutable std::mutex mutex_;  // mutable allows locking in const methods
};

} // namespace discord

#endif // SCRIPT_ENGINE_HPP
