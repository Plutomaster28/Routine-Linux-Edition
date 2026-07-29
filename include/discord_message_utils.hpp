#ifndef DISCORD_MESSAGE_UTILS_HPP
#define DISCORD_MESSAGE_UTILS_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace discord {

// Split message content into ordered, lossless chunks that fit Discord's
// content limit. Breaks prefer line and word boundaries and never split a
// well-formed UTF-8 continuation sequence.
std::vector<std::string> split_discord_message(
    const std::string& content, std::size_t maximum_size = 2000);

} // namespace discord

#endif // DISCORD_MESSAGE_UTILS_HPP
