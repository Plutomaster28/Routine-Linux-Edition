#include "discord_command_utils.hpp"

#include <cassert>
#include <string>

int main() {
    std::string command;
    std::string arguments;

    assert(discord::parse_prefixed_command(
        "~BaLaNcE", "~", command, arguments));
    assert(command == "balance");
    assert(arguments.empty());

    assert(discord::parse_prefixed_command(
        "~pay \t <@123> 250", "~", command, arguments));
    assert(command == "pay");
    assert(arguments == "<@123> 250");

    assert(discord::parse_prefixed_command(
        "~stock\nbuy MEOW 2", "~", command, arguments));
    assert(command == "stock");
    assert(arguments == "buy MEOW 2");

    assert(!discord::parse_prefixed_command(
        "/balance", "~", command, arguments));
    assert(!discord::parse_prefixed_command(
        "~   ", "~", command, arguments));
    assert(!discord::parse_prefixed_command(
        "~balance", "", command, arguments));
}
