#ifndef DISCORD_INTERACTION_UTILS_HPP
#define DISCORD_INTERACTION_UTILS_HPP

#include <string>

#include <nlohmann/json.hpp>

namespace discord {

bool valid_chat_input_command_name(const std::string& name);
std::string render_interaction_arguments(const nlohmann::json& data);

} // namespace discord

#endif // DISCORD_INTERACTION_UTILS_HPP
