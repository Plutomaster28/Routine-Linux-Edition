#include "../include/economy_extension_api.h"
#include "../include/module_interface.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

std::string g_message;
int g_tick_count = 0;
bool g_admin = false;

uint32_t fake_version() { return ECONOMY_EXTENSION_API_VERSION; }

int fake_get_player(const char*, const char*, EconomyPlayerSnapshot* out) {
    *out = {};
    out->wallet_cents = 5000;
    out->checking_cents = 2500;
    out->work_count = 3;
    out->item_quantities[0] = 2;
    return ECONOMY_OK;
}

int fake_claim(const char*, const char*, int64_t, int64_t* award, int64_t* remaining) {
    *award = 7777;
    *remaining = 0;
    return ECONOMY_OK;
}

int fake_move(const char*, const char*, int64_t, int) { return ECONOMY_OK; }
int fake_transfer(const char*, const char*, const char*, int64_t) { return ECONOMY_OK; }
int fake_buy(const char*, const char*, uint32_t, uint32_t, int64_t) { return ECONOMY_OK; }

size_t fake_leaderboard(const char*, EconomyLeaderboardEntry* out, size_t capacity) {
    if (capacity == 0) return 0;
    std::strcpy(out[0].user_id, "200");
    out[0].net_worth_cents = 7500;
    return 1;
}

int fake_game_action(const char*, const char*, const char* action, const char*,
                     int64_t, char* output, size_t capacity) {
    const std::string message = std::string("game action: ") + action;
    const size_t count = std::min(capacity - 1, message.size());
    std::memcpy(output, message.data(), count);
    output[count] = '\0';
    return ECONOMY_OK;
}

int fake_tick_all(int64_t) {
    ++g_tick_count;
    return ECONOMY_OK;
}

int fake_get_currency(const char*, char* symbol, size_t symbol_capacity,
                      char* name, size_t name_capacity) {
    std::strncpy(symbol, "¤", symbol_capacity - 1);
    std::strncpy(name, "Chaos Buck", name_capacity - 1);
    return ECONOMY_OK;
}

void send_message(void*, const char*, const char* content) {
    g_message = content ? content : "";
}

void log_message(const char*, const char*) {}
uint64_t uptime(void*) { return 0; }

const char* guild_id(void*, const char* channel_id) {
    return channel_id && std::string(channel_id) == "300" ? "100" : nullptr;
}

const char* user_roles(void*, const char*, const char*) { return "10,20"; }
int is_admin(void*, const char*, const char*) { return g_admin ? 1 : 0; }

void* extension_function(void*, const char* name) {
    if (!name) return nullptr;
    const std::string function_name(name);
    if (function_name == "economy_api_version") return reinterpret_cast<void*>(fake_version);
    if (function_name == "economy_get_player") return reinterpret_cast<void*>(fake_get_player);
    if (function_name == "economy_claim_daily") return reinterpret_cast<void*>(fake_claim);
    if (function_name == "economy_work") return reinterpret_cast<void*>(fake_claim);
    if (function_name == "economy_move_money") return reinterpret_cast<void*>(fake_move);
    if (function_name == "economy_transfer") return reinterpret_cast<void*>(fake_transfer);
    if (function_name == "economy_buy_item") return reinterpret_cast<void*>(fake_buy);
    if (function_name == "economy_leaderboard") return reinterpret_cast<void*>(fake_leaderboard);
    if (function_name == "economy_game_action") return reinterpret_cast<void*>(fake_game_action);
    if (function_name == "economy_tick_all") return reinterpret_cast<void*>(fake_tick_all);
    if (function_name == "economy_get_currency") return reinterpret_cast<void*>(fake_get_currency);
    return nullptr;
}

struct Library {
#ifdef _WIN32
    HMODULE handle = nullptr;
#else
    void* handle = nullptr;
#endif

    explicit Library(const char* path) {
#ifdef _WIN32
        handle = LoadLibraryA(path);
#else
        handle = dlopen(path, RTLD_NOW);
#endif
        assert(handle);
    }

    ~Library() {
#ifdef _WIN32
        if (handle) FreeLibrary(handle);
#else
        if (handle) dlclose(handle);
#endif
    }

    void* symbol(const char* name) const {
#ifdef _WIN32
        return reinterpret_cast<void*>(GetProcAddress(handle, name));
#else
        return dlsym(handle, name);
#endif
    }
};

const CommandRegistration* find_command(const CommandRegistration* commands,
                                        const std::string& name) {
    for (size_t i = 0; commands[i].name; ++i) {
        if (name == commands[i].name) return &commands[i];
    }
    return nullptr;
}

}  // namespace

int main(int argc, char** argv) {
    assert(argc == 2);
    Library library(argv[1]);
    auto get_info = reinterpret_cast<ModuleInfo (*)()>(library.symbol("module_get_info"));
    auto initialize = reinterpret_cast<int (*)(const KernelBridge*, void*)>(
        library.symbol("module_init"));
    auto shutdown = reinterpret_cast<void (*)()>(library.symbol("module_shutdown"));
    auto register_commands = reinterpret_cast<const CommandRegistration* (*)()>(
        library.symbol("module_register_commands"));
    auto on_tick = reinterpret_cast<void (*)(void*)>(library.symbol("module_on_tick"));
    assert(get_info && initialize && shutdown && register_commands && on_tick);
    assert(get_info().api_version == MODULE_API_VERSION);
    assert(std::string(get_info().version) == "2.0.0");

    KernelBridge bridge{};
    bridge.send_message = send_message;
    bridge.log = log_message;
    bridge.get_uptime = uptime;
    bridge.get_guild_id = guild_id;
    bridge.get_extension_function = extension_function;
    bridge.get_user_roles = user_roles;
    bridge.is_guild_admin = is_admin;
    assert(initialize(&bridge, nullptr) == 0);
    on_tick(nullptr);
    assert(g_tick_count == 1);

    const CommandRegistration* commands = register_commands();
    const CommandRegistration* economy = find_command(commands, "economy");
    const CommandRegistration* balance = find_command(commands, "balance");
    const CommandRegistration* forex = find_command(commands, "forex");
    const CommandRegistration* daily = find_command(commands, "daily");
    const CommandRegistration* inventory = find_command(commands, "inventory");
    const CommandRegistration* leaderboard = find_command(commands, "leaderboard");
    const CommandRegistration* profile = find_command(commands, "profile");
    const CommandRegistration* stock = find_command(commands, "stock");
    const CommandRegistration* derivatives = find_command(commands, "derivatives");
    const CommandRegistration* bankruptcy = find_command(commands, "bankruptcy");
    const CommandRegistration* orders = find_command(commands, "orders");
    const CommandRegistration* contract = find_command(commands, "contract");
    const CommandRegistration* casino = find_command(commands, "casino");
    const CommandRegistration* history = find_command(commands, "history");
    const CommandRegistration* stipend = find_command(commands, "stipend");
    const CommandRegistration* corporate = find_command(commands, "corporate");
    const CommandRegistration* econadmin = find_command(commands, "econadmin");
    const CommandRegistration* crime = find_command(commands, "crime");
    const CommandRegistration* election = find_command(commands, "election");
    const CommandRegistration* government = find_command(commands, "government");
    const CommandRegistration* economystats = find_command(commands, "economystats");
    const CommandRegistration* chart = find_command(commands, "chart");
    const CommandRegistration* rank = find_command(commands, "rank");
    const CommandRegistration* auction = find_command(commands, "auction");
    const CommandRegistration* supply = find_command(commands, "supply");
    const CommandRegistration* produce = find_command(commands, "produce");
    const CommandRegistration* equipment = find_command(commands, "equipment");
    const CommandRegistration* marketing = find_command(commands, "marketing");
    const CommandRegistration* businessloan = find_command(commands, "businessloan");
    const CommandRegistration* partnership = find_command(commands, "partnership");
    const CommandRegistration* fundamentals = find_command(commands, "fundamentals");
    const CommandRegistration* certifications = find_command(commands, "certifications");
    const CommandRegistration* skills = find_command(commands, "skills");
    const CommandRegistration* agreement = find_command(commands, "agreement");
    const CommandRegistration* mystats = find_command(commands, "mystats");
    const CommandRegistration* mychart = find_command(commands, "mychart");
    assert(economy && balance && forex && daily && inventory && leaderboard && profile && stock &&
           derivatives && bankruptcy && orders && contract && casino && history &&
           stipend && corporate && econadmin && crime && election && government &&
           economystats && chart && rank && auction && supply && produce &&
           equipment && marketing && businessloan && partnership);
    assert(fundamentals);
    assert(certifications && skills && agreement && mystats && mychart);

    economy->callback(nullptr, "300", "200", "");
    assert(g_message.find("Routine Economy · Version 2") != std::string::npos);
    assert(g_message.find("Legacy players can replace `/` with `~`") !=
           std::string::npos);
    assert(g_message.size() <= 2000);
    forex->callback(nullptr, "300", "200", "markets");
    assert(g_message == "game action: forex");

    balance->callback(nullptr, "300", "200", "");
    assert(g_message.find("Wallet: **¤50.00**") != std::string::npos);
    assert(g_message.find("Net worth: **¤75.00**") != std::string::npos);

    daily->callback(nullptr, "300", "200", "");
    assert(g_message.find("¤77.77") != std::string::npos);

    inventory->callback(nullptr, "300", "200", "");
    assert(g_message.find("Emergency Ramen: **2**") != std::string::npos);

    leaderboard->callback(nullptr, "300", "200", "");
    assert(g_message.find("<@200>") != std::string::npos);

    profile->callback(nullptr, "300", "200", "");
    assert(g_message == "game action: profile");
    stock->callback(nullptr, "300", "200", "buy MEOW 1");
    assert(g_message == "game action: stock");
    stipend->callback(nullptr, "300", "200", "");
    assert(g_message == "game action: stipend");
    g_admin = false;
    econadmin->callback(nullptr, "300", "200", "status");
    assert(g_message.find("Only the server owner") != std::string::npos);
    g_admin = true;
    econadmin->callback(nullptr, "300", "200", "status");
    assert(g_message == "game action: admin");
    election->callback(nullptr, "300", "200", "status");
    assert(g_message == "game action: election");
    chart->callback(nullptr, "300", "200", "market 12");
    assert(g_message == "game action: chart");

    balance->callback(nullptr, "DM", "200", "");
    assert(g_message.find("unavailable in DMs") != std::string::npos);

    shutdown();
    std::cout << "economy module tests passed\n";
    return 0;
}
