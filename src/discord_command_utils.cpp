#include "discord_command_utils.hpp"

#include <algorithm>
#include <cctype>

namespace discord {

bool parse_prefixed_command(const std::string& content,
                            const std::string& prefix,
                            std::string& command,
                            std::string& arguments) {
    command.clear();
    arguments.clear();
    if (prefix.empty() || content.size() <= prefix.size() ||
        content.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }

    const std::string without_prefix = content.substr(prefix.size());
    const size_t separator = without_prefix.find_first_of(" \t\r\n");
    if (separator == std::string::npos) {
        command = without_prefix;
    } else {
        command = without_prefix.substr(0, separator);
        const size_t arguments_start = without_prefix.find_first_not_of(
            " \t\r\n", separator);
        if (arguments_start != std::string::npos) {
            arguments = without_prefix.substr(arguments_start);
        }
    }

    std::transform(command.begin(), command.end(), command.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return !command.empty();
}

}  // namespace discord
