#include "discord_interaction_utils.hpp"

#include <cassert>

using nlohmann::json;

int main() {
    assert(discord::valid_chat_input_command_name("balance"));
    assert(discord::valid_chat_input_command_name("economy-stats_2"));
    assert(!discord::valid_chat_input_command_name(""));
    assert(!discord::valid_chat_input_command_name("Uppercase"));
    assert(!discord::valid_chat_input_command_name(std::string(33, 'x')));

    const json compatibility = {
        {"options", json::array({
            {
                {"name", "arguments"},
                {"type", 3},
                {"value", "buy RAT 5"}
            }
        })}
    };
    assert(discord::render_interaction_arguments(compatibility) ==
           "buy RAT 5");

    const json typed = {
        {"options", json::array({
            {{"name", "user"}, {"type", 6}, {"value", "123"}},
            {{"name", "amount"}, {"type", 4}, {"value", 500}},
            {{"name", "confirmed"}, {"type", 5}, {"value", true}}
        })}
    };
    assert(discord::render_interaction_arguments(typed) ==
           "<@123> 500 true");

    const json nested = {
        {"options", json::array({
            {
                {"name", "orders"},
                {"type", 1},
                {"options", json::array({
                    {{"name", "action"}, {"type", 3}, {"value", "list"}}
                })}
            }
        })}
    };
    assert(discord::render_interaction_arguments(nested) == "orders list");
    assert(discord::render_interaction_arguments(json::object()).empty());
    return 0;
}
