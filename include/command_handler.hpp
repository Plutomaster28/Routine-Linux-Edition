#pragma once

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace discord {

using json = nlohmann::json;

// Forward declarations
class DiscordBot;

class CommandHandler {
public:
    using CommandCallback = std::function<void(const json& message, const std::string& args)>;
    
    CommandHandler(const std::string& prefix = "~");
    ~CommandHandler();
    
    // Set bot instance for module command forwarding
    void set_bot(DiscordBot* bot) { bot_ = bot; }
    
    // Register a command with a callback
    void register_command(const std::string& command, CommandCallback callback);
    void register_command(const std::string& command,
                          const std::string& description,
                          bool accepts_arguments,
                          CommandCallback callback);
    
    // Process a message and execute command if found
    bool handle_message(const json& message);

    // Process a Discord APPLICATION_COMMAND interaction.
    bool handle_interaction(const json& interaction);

    // Build the complete Discord bulk-overwrite payload after modules load.
    json build_application_commands() const;
    
    // Get the command prefix
    std::string get_prefix() const { return prefix_; }
    
private:
    struct RegisteredCommand {
        CommandCallback callback;
        std::string description;
        bool accepts_arguments = true;
    };

    std::string prefix_;
    std::unordered_map<std::string, RegisteredCommand> commands_;
    DiscordBot* bot_;
    
    static std::string interaction_arguments(const json& data);
};

} // namespace discord
