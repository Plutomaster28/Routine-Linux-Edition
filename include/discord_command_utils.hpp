#pragma once

#include <string>

namespace discord {

// Parse a legacy text command while keeping transport concerns outside the
// command registry. Command names are normalized to lowercase.
bool parse_prefixed_command(const std::string& content,
                            const std::string& prefix,
                            std::string& command,
                            std::string& arguments);

}  // namespace discord
