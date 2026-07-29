#include "discord_message_utils.hpp"

#include <cassert>
#include <string>
#include <vector>

namespace {

void assert_lossless_and_bounded(const std::string& original) {
    const std::vector<std::string> chunks =
        discord::split_discord_message(original);
    std::string reassembled;
    for (const std::string& chunk : chunks) {
        assert(!chunk.empty());
        assert(chunk.size() <= 2000);
        reassembled += chunk;
    }
    assert(reassembled == original);
}

} // namespace

int main() {
    const std::string short_message = "Routine is alive.";
    const auto short_chunks = discord::split_discord_message(short_message);
    assert(short_chunks.size() == 1);
    assert(short_chunks.front() == short_message);

    assert_lossless_and_bounded(std::string(2000, 'x'));
    assert_lossless_and_bounded(std::string(2001, 'x'));

    std::string help_page;
    for (int index = 0; index < 150; ++index) {
        help_page += "• `~command" + std::to_string(index) +
                     "` - Perform some chaotic economy action\n";
    }
    assert_lossless_and_bounded(help_page);

    std::string utf8_message;
    for (int index = 0; index < 600; ++index) {
        utf8_message += "\xF0\x9F\xA6\x90"; // U+1F990 SHRIMP
    }
    assert_lossless_and_bounded(utf8_message);

    assert(discord::split_discord_message("", 2000).empty());
    assert(discord::split_discord_message("content", 0).empty());
    return 0;
}
