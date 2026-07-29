#include "script_engine.hpp"
#include "discord_bot.hpp"
#include "module_loader.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <regex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace discord {

// === ScriptContext Implementation ===

ScriptContext::ScriptContext() {
    data_ = nlohmann::json::object();
}

void ScriptContext::set(const std::string& key, const nlohmann::json& value) {
    // Support nested keys like "author.name"
    std::vector<std::string> parts;
    std::string current;
    for (char c : key) {
        if (c == '.') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    
    if (parts.empty()) return;
    
    // Navigate to the right place in JSON
    nlohmann::json* target = &data_;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (!target->contains(parts[i])) {
            (*target)[parts[i]] = nlohmann::json::object();
        }
        target = &(*target)[parts[i]];
    }
    
    (*target)[parts.back()] = value;
}

nlohmann::json ScriptContext::get(const std::string& key) const {
    // Support nested keys
    std::vector<std::string> parts;
    std::string current;
    for (char c : key) {
        if (c == '.') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    
    const nlohmann::json* target = &data_;
    for (const auto& part : parts) {
        if (!target->contains(part)) {
            return nlohmann::json();
        }
        target = &(*target)[part];
    }
    
    return *target;
}

bool ScriptContext::has(const std::string& key) const {
    return !get(key).is_null();
}

std::string ScriptContext::substitute(const std::string& text) const {
    std::string result = text;
    std::regex template_regex(R"(\{\{([^}]+)\}\})");
    std::smatch match;
    
    std::string working = text;
    result.clear();
    
    while (std::regex_search(working, match, template_regex)) {
        result += match.prefix();
        
        std::string key = match[1].str();
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        
        nlohmann::json value = get(key);
        if (!value.is_null()) {
            if (value.is_string()) {
                result += value.get<std::string>();
            } else {
                result += value.dump();
            }
        } else {
            // Keep the template if not found
            result += "{{" + key + "}}";
        }
        
        working = match.suffix();
    }
    result += working;
    
    return result;
}

void ScriptContext::set_event_data(const nlohmann::json& event) {
    // Copy event data into context
    for (auto& [key, value] : event.items()) {
        set(key, value);
    }
}

// === ScriptCondition Implementation ===

bool ScriptCondition::evaluate(const ScriptContext& ctx) const {
    std::string field_value;
    auto val = ctx.get(field);
    if (val.is_string()) {
        field_value = val.get<std::string>();
    } else if (!val.is_null()) {
        field_value = val.dump();
    }
    
    switch (type) {
        case STARTS_WITH:
            return field_value.find(value) == 0;
            
        case ENDS_WITH:
            if (value.length() > field_value.length()) return false;
            return field_value.compare(field_value.length() - value.length(), 
                                      value.length(), value) == 0;
            
        case CONTAINS:
            return field_value.find(value) != std::string::npos;
            
        case EQUALS:
            return field_value == value;
            
        case MATCHES_REGEX: {
            try {
                std::regex regex(value);
                return std::regex_search(field_value, regex);
            } catch (...) {
                return false;
            }
        }
            
        case HAS_ROLE: {
            auto roles = ctx.get("author.roles");
            if (roles.is_array()) {
                for (const auto& role : roles) {
                    if (role.is_string() && role.get<std::string>() == value) {
                        return true;
                    }
                }
            }
            return false;
        }
            
        case IN_CHANNEL: {
            auto channel_id = ctx.get("channel_id");
            if (channel_id.is_string()) {
                return channel_id.get<std::string>() == value;
            }
            return false;
        }
            
        case ALWAYS:
            return true;
    }
    
    return false;
}

// === ScriptAction Implementation ===

bool ScriptAction::execute(ScriptContext& ctx, ModuleLoader* loader, DiscordBot* bot) const {
    (void)loader;
    // This is where we'd invoke the module
    // For now, we'll implement basic support
    
    std::cout << "  → Executing action: module=" << module << std::endl;
    
    // Substitute all template variables in arguments
    std::map<std::string, std::string> resolved_args;
    for (const auto& [key, value] : args) {
        resolved_args[key] = ctx.substitute(value);
        std::cout << "    " << key << " = " << resolved_args[key] << std::endl;
    }
    
    // Special built-in modules
    if (module == "responder") {
        // Send a message
        if (resolved_args.count("channel") && resolved_args.count("content")) {
            bot->send_message(resolved_args["channel"], resolved_args["content"]);
            return true;
        }
    } else if (module == "log") {
        // Log a message
        if (resolved_args.count("message")) {
            std::cout << "[SCRIPT LOG] " << resolved_args["message"] << std::endl;
            return true;
        }
    }
    
    // TODO: Invoke actual module via loader
    // This would call loader->invoke_module(module, resolved_args, ctx)
    
    return true;
}

// === Script Implementation ===

bool Script::should_run(const ScriptContext& ctx) const {
    if (!enabled) return false;
    
    // All conditions must pass
    for (const auto& condition : conditions) {
        if (!condition.evaluate(ctx)) {
            return false;
        }
    }
    
    return true;
}

bool Script::execute(ScriptContext& ctx, ModuleLoader* loader, DiscordBot* bot) const {
    std::cout << "Executing script: " << name << std::endl;
    
    for (const auto& action : actions) {
        if (!action.execute(ctx, loader, bot)) {
            std::cerr << "Action failed in script: " << name << std::endl;
            return false;
        }
    }
    
    return true;
}

// === ScriptParser Implementation ===

std::unique_ptr<Script> ScriptParser::parse(const std::string& text, std::string& error) {
    try {
        // Parse YAML
        YAML::Node doc = YAML::Load(text);
        
        auto script = std::make_unique<Script>();
        script->enabled = true;
        
        // Parse script name
        if (!doc["script"]) {
            error = "Missing 'script' field (script name)";
            return nullptr;
        }
        script->name = doc["script"].as<std::string>();
        
        // Parse event type (on: field)
        if (!doc["on"]) {
            error = "Missing 'on' field (event type)";
            return nullptr;
        }
        script->event_type = doc["on"].as<std::string>();
        
        // Parse conditions (when: field) - optional
        if (doc["when"]) {
            YAML::Node when = doc["when"];
            
            // Each key in when is a condition
            for (auto it = when.begin(); it != when.end(); ++it) {
                ScriptCondition cond;
                std::string cond_type = it->first.as<std::string>();
                std::string cond_value = it->second.as<std::string>();
                
                cond.type = string_to_condition_type(cond_type);
                cond.field = "content";  // Default field for most conditions
                cond.value = cond_value;
                
                // Special cases
                if (cond_type == "has_role") {
                    cond.field = "author.roles";
                } else if (cond_type == "in_channel") {
                    cond.field = "channel_id";
                }
                
                script->conditions.push_back(cond);
            }
        }
        
        // Parse actions (do: field)
        if (!doc["do"]) {
            error = "Missing 'do' field (actions)";
            return nullptr;
        }
        
        YAML::Node actions = doc["do"];
        if (!actions.IsSequence()) {
            error = "'do' field must be a list of actions";
            return nullptr;
        }
        
        for (size_t i = 0; i < actions.size(); ++i) {
            YAML::Node action_node = actions[i];
            
            if (!action_node["module"]) {
                error = "Action missing 'module' field";
                return nullptr;
            }
            
            ScriptAction action;
            action.module = action_node["module"].as<std::string>();
            
            // Parse args if present
            if (action_node["args"]) {
                YAML::Node args = action_node["args"];
                for (auto it = args.begin(); it != args.end(); ++it) {
                    std::string key = it->first.as<std::string>();
                    std::string value = it->second.as<std::string>();
                    action.args[key] = value;
                }
            }
            
            script->actions.push_back(action);
        }
        
        return script;
        
    } catch (const YAML::Exception& e) {
        error = std::string("YAML parse error: ") + e.what();
        return nullptr;
    } catch (const std::exception& e) {
        error = std::string("Parse error: ") + e.what();
        return nullptr;
    }
}

ScriptCondition::Type ScriptParser::string_to_condition_type(const std::string& type) {
    if (type == "starts_with") return ScriptCondition::STARTS_WITH;
    if (type == "ends_with") return ScriptCondition::ENDS_WITH;
    if (type == "contains") return ScriptCondition::CONTAINS;
    if (type == "equals") return ScriptCondition::EQUALS;
    if (type == "matches_regex") return ScriptCondition::MATCHES_REGEX;
    if (type == "has_role") return ScriptCondition::HAS_ROLE;
    if (type == "in_channel") return ScriptCondition::IN_CHANNEL;
    return ScriptCondition::ALWAYS;
}

// === ScriptEngine Implementation ===

ScriptEngine::ScriptEngine(DiscordBot* bot, ModuleLoader* loader)
    : bot_(bot), loader_(loader) {
}

ScriptEngine::~ScriptEngine() {
}

bool ScriptEngine::load_script(const std::string& text, const std::string& author, std::string& error) {
    auto script = ScriptParser::parse(text, error);
    if (!script) {
        return false;
    }
    
    script->author = author;
    
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm time_parts{};
#ifdef _WIN32
    localtime_s(&time_parts, &time_t);
#else
    localtime_r(&time_t, &time_parts);
#endif
    std::stringstream ss;
    ss << std::put_time(&time_parts, "%Y-%m-%d %H:%M:%S");
    script->created_at = ss.str();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if script already exists
    if (scripts_.count(script->name)) {
        error = "Script '" + script->name + "' already exists. Remove it first.";
        return false;
    }
    
    std::cout << "Loaded script: " << script->name 
              << " (event: " << script->event_type << ")" << std::endl;
    
    scripts_[script->name] = std::move(script);
    return true;
}

bool ScriptEngine::enable_script(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = scripts_.find(name);
    if (it == scripts_.end()) return false;
    it->second->enabled = true;
    return true;
}

bool ScriptEngine::disable_script(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = scripts_.find(name);
    if (it == scripts_.end()) return false;
    it->second->enabled = false;
    return true;
}

bool ScriptEngine::remove_script(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return scripts_.erase(name) > 0;
}

std::vector<std::string> ScriptEngine::list_scripts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& [name, script] : scripts_) {
        names.push_back(name);
    }
    return names;
}

const Script* ScriptEngine::get_script(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = scripts_.find(name);
    if (it == scripts_.end()) return nullptr;
    return it->second.get();
}

void ScriptEngine::handle_event(const std::string& event_type, const nlohmann::json& event_data) {
    // Get all scripts for this event type
    std::vector<Script*> matching_scripts;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [name, script] : scripts_) {
            if (script->event_type == event_type && script->enabled) {
                matching_scripts.push_back(script.get());
            }
        }
    }
    
    if (matching_scripts.empty()) return;
    
    // Execute matching scripts
    for (Script* script : matching_scripts) {
        ScriptContext ctx;
        ctx.set_event_data(event_data);
        
        if (script->should_run(ctx)) {
            script->execute(ctx, loader_, bot_);
        }
    }
}

bool ScriptEngine::save_scripts(const std::string& path) {
    (void)path;
    // TODO: Implement persistence
    return false;
}

bool ScriptEngine::load_scripts(const std::string& path) {
    (void)path;
    // TODO: Implement persistence
    return false;
}

} // namespace discord
