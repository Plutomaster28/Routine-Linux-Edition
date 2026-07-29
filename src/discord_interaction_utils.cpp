#include "discord_interaction_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <vector>

namespace discord {

bool valid_chat_input_command_name(const std::string& name) {
    return !name.empty() && name.size() <= 32 &&
           std::all_of(name.begin(), name.end(), [](unsigned char value) {
               return std::islower(value) || std::isdigit(value) ||
                      value == '-' || value == '_';
           });
}

std::string render_interaction_arguments(const nlohmann::json& data) {
    using json = nlohmann::json;
    if (!data.contains("options") || !data["options"].is_array()) return "";

    std::vector<std::string> values;
    const auto append_options = [&](const auto& self,
                                    const json& options) -> void {
        for (const auto& option : options) {
            if (!option.is_object()) continue;
            if (option.contains("options") &&
                option["options"].is_array()) {
                values.push_back(option.value("name", ""));
                self(self, option["options"]);
                continue;
            }
            if (!option.contains("value")) continue;
            const int type = option.value("type", 3);
            const json& value = option["value"];
            std::string rendered;
            if (value.is_string()) {
                rendered = value.get<std::string>();
            } else if (value.is_boolean()) {
                rendered = value.get<bool>() ? "true" : "false";
            } else if (value.is_number_integer()) {
                rendered = std::to_string(value.get<int64_t>());
            } else if (value.is_number_float()) {
                std::ostringstream stream;
                stream << value.get<double>();
                rendered = stream.str();
            }
            if (type == 6 && !rendered.empty()) {
                rendered = "<@" + rendered + ">";
            } else if (type == 7 && !rendered.empty()) {
                rendered = "<#" + rendered + ">";
            } else if (type == 8 && !rendered.empty()) {
                rendered = "<@&" + rendered + ">";
            }
            values.push_back(std::move(rendered));
        }
    };
    append_options(append_options, data["options"]);

    if (data["options"].size() == 1 &&
        data["options"][0].value("name", "") == "arguments" &&
        !values.empty()) {
        return values.front();
    }

    std::ostringstream joined;
    bool first = true;
    for (const std::string& value : values) {
        if (value.empty()) continue;
        if (!first) joined << ' ';
        joined << value;
        first = false;
    }
    return joined.str();
}

} // namespace discord
