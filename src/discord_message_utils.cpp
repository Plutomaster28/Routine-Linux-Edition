#include "discord_message_utils.hpp"

#include <algorithm>

namespace discord {
namespace {

bool is_utf8_continuation(unsigned char byte) {
    return (byte & 0xc0U) == 0x80U;
}

} // namespace

std::vector<std::string> split_discord_message(
    const std::string& content, std::size_t maximum_size) {
    std::vector<std::string> chunks;
    if (content.empty() || maximum_size == 0) {
        return chunks;
    }

    std::size_t offset = 0;
    while (content.size() - offset > maximum_size) {
        std::size_t boundary = offset + maximum_size;

        // A byte limit is conservative for Discord's character limit, and
        // backing up here keeps a multi-byte UTF-8 character intact.
        while (boundary > offset &&
               is_utf8_continuation(
                   static_cast<unsigned char>(content[boundary]))) {
            --boundary;
        }
        if (boundary == offset) {
            // Malformed UTF-8 (or an impractically tiny custom test limit):
            // guarantee forward progress while still respecting the limit.
            boundary = offset + maximum_size;
        }

        const std::size_t minimum_preferred_break =
            offset + (boundary - offset) / 2;
        const std::size_t newline = content.rfind('\n', boundary - 1);
        const std::size_t space = content.rfind(' ', boundary - 1);

        std::size_t split = boundary;
        if (newline != std::string::npos &&
            newline >= minimum_preferred_break) {
            split = newline + 1;
        } else if (space != std::string::npos &&
                   space >= minimum_preferred_break) {
            split = space + 1;
        }

        chunks.emplace_back(content.substr(offset, split - offset));
        offset = split;
    }

    if (offset < content.size()) {
        chunks.emplace_back(content.substr(offset));
    }
    return chunks;
}

} // namespace discord
