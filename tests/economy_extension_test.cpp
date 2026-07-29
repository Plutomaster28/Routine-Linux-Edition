#include "../include/economy_extension_api.h"
#include "../include/extension_interface.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

void test_log(const char*) {}
uint64_t test_uptime() { return 0; }
void* test_allocate(size_t size) { return std::malloc(size); }
void test_deallocate(void* memory) { std::free(memory); }

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

template <typename Function>
Function extension_function(const ExtensionAPI* api, const char* name) {
    Function function = reinterpret_cast<Function>(api->get_function(name));
    assert(function);
    return function;
}

}  // namespace

int main(int argc, char** argv) {
    assert(argc == 3);
    const std::filesystem::path database = argv[2];
    std::error_code ec;
    std::filesystem::remove(database, ec);
    std::filesystem::remove(database.string() + ".tmp", ec);
    std::filesystem::remove(database.string() + ".previous", ec);

#ifdef _WIN32
    _putenv_s("ROUTINE_ECONOMY_DATA", database.string().c_str());
#else
    setenv("ROUTINE_ECONOMY_DATA", database.string().c_str(), 1);
#endif

    {
        Library library(argv[1]);
        auto initialize = reinterpret_cast<extension_init_func>(
            library.symbol(EXTENSION_INIT_NAME));
        auto shutdown = reinterpret_cast<extension_shutdown_func>(
            library.symbol(EXTENSION_SHUTDOWN_NAME));
        auto get_api = reinterpret_cast<extension_get_api_func>(
            library.symbol(EXTENSION_GET_API_NAME));
        auto get_info = reinterpret_cast<extension_get_info_func>(
            library.symbol(EXTENSION_GET_INFO_NAME));
        assert(initialize && shutdown && get_api && get_info);
        assert(std::string(get_info()->version) == "1.0.0");

        ExtensionKernelBridge bridge{};
        bridge.log = test_log;
        bridge.get_uptime = test_uptime;
        bridge.allocate = test_allocate;
        bridge.deallocate = test_deallocate;
        assert(initialize(&bridge) == 0);

        const ExtensionAPI* api = get_api();
        assert(api && api->get_function);
        auto version = extension_function<economy_api_version_func>(
            api, "economy_api_version");
        auto get_player = extension_function<economy_get_player_func>(
            api, "economy_get_player");
        auto daily = extension_function<economy_claim_func>(
            api, "economy_claim_daily");
        auto work = extension_function<economy_claim_func>(
            api, "economy_work");
        auto move = extension_function<economy_move_money_func>(
            api, "economy_move_money");
        auto transfer = extension_function<economy_transfer_func>(
            api, "economy_transfer");
        auto buy = extension_function<economy_buy_item_func>(
            api, "economy_buy_item");
        auto leaderboard = extension_function<economy_leaderboard_func>(
            api, "economy_leaderboard");
        auto game_action = extension_function<economy_game_action_func>(
            api, "economy_game_action");
        auto tick_all = extension_function<economy_tick_all_func>(
            api, "economy_tick_all");
        auto get_currency = extension_function<economy_get_currency_func>(
            api, "economy_get_currency");

        assert(version() == ECONOMY_EXTENSION_API_VERSION);

        EconomyPlayerSnapshot first{};
        EconomyPlayerSnapshot isolated{};
        assert(get_player("100", "200", &first) == ECONOMY_OK);
        assert(first.wallet_cents == 5000 && first.checking_cents == 0);
        assert(get_player("101", "200", &isolated) == ECONOMY_OK);
        assert(isolated.wallet_cents == 5000);

        int64_t award = 0;
        int64_t remaining = 0;
        assert(daily("100", "200", 1000000, &award, &remaining) == ECONOMY_OK);
        assert(award >= 7500 && award <= 12500);
        assert(daily("100", "200", 1000001, &award, &remaining) == ECONOMY_COOLDOWN);
        assert(remaining == 86399);
        assert(work("100", "200", 1000000, &award, &remaining) == ECONOMY_OK);
        assert(award >= 1500 && award <= 4000);

        assert(move("100", "200", 1000, 1) == ECONOMY_OK);
        assert(move("100", "200", 250, 0) == ECONOMY_OK);
        assert(transfer("100", "200", "201", 500) == ECONOMY_OK);
        assert(transfer("100", "200", "200", 500) == ECONOMY_SELF_TRANSFER);
        assert(buy("100", "201", 1, 1, 475) == ECONOMY_OK);

        EconomyPlayerSnapshot recipient{};
        assert(get_player("100", "201", &recipient) == ECONOMY_OK);
        assert(recipient.item_quantities[1] == 1);
        assert(recipient.wallet_cents == 5025);

        EconomyLeaderboardEntry entries[ECONOMY_MAX_LEADERBOARD]{};
        const size_t count = leaderboard("100", entries, ECONOMY_MAX_LEADERBOARD);
        assert(count == 2);
        assert(std::string(entries[0].user_id) == "200");

        char game_output[1900]{};
        assert(game_action("100", "200", "card", "charge 10", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "repay", "10", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "loan", "15000", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("approved") != std::string::npos);
        assert(game_action("100", "200", "enroll", "1", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "savings", "deposit 100", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "stock", "buy MEOW 2", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "derivatives", "call DOGM 10", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "bonds", "buy 100", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "business", "start", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "property", "buy 1", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "market", "", 1000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("DOGM") != std::string::npos);
        assert(game_action("100", "200", "economyinfo", "", 1003600,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Inflation") != std::string::npos);
        assert(game_action("100", "200", "cd", "open 100", 1003600,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "cd", "close", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "invest", "index buy 100", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "invest", "retirement buy 100", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "invest", "commodity buy 100", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "orders", "place sell MEOW 1 9999", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "orders", "cancel 1", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "short", "open MEOW 1", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "short", "close MEOW 1", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "collectible", "buy 1", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "insurance", "buy 1", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "insurance", "claim", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "stipend", "10,20", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "corporate", "status", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "payroll", "inventory 2", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "contract", "offer 201 100 110", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "201", "contract", "accept 1", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "201", "contract", "pay 1", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "201", "loan", "5000", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "201", "casino", "start", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("100", "200", "orders", "place sell MEOW 1 9999", 1003601,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(tick_all(1007201) == ECONOMY_OK);

        // Server configuration changes only future accounts and survives restarts.
        assert(game_action("102", "900", "admin", "currency ¤ Chaos Bucks", 2000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("102", "900", "admin", "starting 1", 2000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        char symbol[8]{};
        char currency_name[32]{};
        assert(get_currency("102", symbol, sizeof(symbol), currency_name,
                            sizeof(currency_name)) == ECONOMY_OK);
        assert(std::string(symbol) == "¤");
        assert(std::string(currency_name) == "Chaos Bucks");
        EconomyPlayerSnapshot configured{};
        assert(get_player("102", "300", &configured) == ECONOMY_OK);
        assert(configured.wallet_cents == 100);

        // Capital Two accepts damaged credit; three missed daily minimums default.
        assert(game_action("102", "300", "bank", "select 6", 2000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("102", "300", "loan", "10", 2000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(transfer("102", "300", "301", 1100) == ECONOMY_OK);
        assert(tick_all(2000000 + 3 * 24 * 3600) == ECONOMY_OK);
        assert(game_action("102", "300", "bank", "", 2259200,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Defaults: **1**") != std::string::npos);

        // Elections deterministically install policy and feed business/tax systems.
        assert(game_action("103", "999", "admin", "starting 500", 3000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        EconomyPlayerSnapshot candidate_account{};
        assert(get_player("103", "400", &candidate_account) == ECONOMY_OK);
        assert(candidate_account.wallet_cents == 50000);
        assert(game_action("103", "400", "election", "run business", 3000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("103", "400", "election", "vote 400", 3000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(tick_all(3000000 + 24 * 3600) == ECONOMY_OK);
        assert(game_action("103", "400", "government", "", 3086400,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("<@400>") != std::string::npos);
        assert(std::string(game_output).find("Pro-Business") != std::string::npos);
        assert(game_action("103", "400", "taxes", "pay 10", 3086400,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(tick_all(3090000) == ECONOMY_OK);
        assert(game_action("103", "400", "economystats", "", 3090000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Money supply") != std::string::npos);
        assert(std::string(game_output).find("Bank stability") != std::string::npos);
        assert(game_action("103", "400", "chart", "market 2", 3090000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("hourly points") != std::string::npos);
        assert(game_action("103", "400", "rank", "taxes", 3090000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("<@400>") != std::string::npos);

        // Crime always produces a persistent outcome, even though success is random.
        assert(game_action("103", "401", "crime", "pickpocket", 3086400,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("succeeded") != std::string::npos ||
               std::string(game_output).find("Caught") != std::string::npos);
        assert(game_action("103", "401", "crime", "status", 3086401,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Offenses: **1**") != std::string::npos);

        // Treasury-backed welfare is eligibility- and cooldown-gated.
        assert(game_action("104", "500", "admin", "starting 1", 4000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(get_player("104", "501", &candidate_account) == ECONOMY_OK);
        assert(game_action("104", "501", "welfare", "", 4000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("104", "501", "welfare", "", 4000001,
                           game_output, sizeof(game_output)) == ECONOMY_COOLDOWN);

        // Dynamic property and the auction house use durable asset/bid escrow.
        assert(game_action("104", "500", "admin", "starting 50000", 4000010,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("104", "600", "collectible", "buy 3", 4000011,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("104", "600", "auction", "list relic 2 20 30 24", 4000012,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("#1") != std::string::npos);
        assert(game_action("104", "601", "auction", "bid 1 25", 4000013,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("104", "601", "auction", "buyout 1", 4000014,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("104", "601", "collectible", "", 4000015,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Launch Relics:** 2") != std::string::npos);

        assert(game_action("104", "600", "property", "buy 1 cash", 4000016,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Apartment #") != std::string::npos);
        assert(game_action("104", "600", "property", "", 4000017,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        const std::string property_status = game_output;
        const size_t property_marker = property_status.find("#");
        assert(property_marker != std::string::npos);
        const uint64_t property_id = std::strtoull(
            property_status.c_str() + property_marker + 1, nullptr, 10);
        const std::string property_listing = "list property " +
            std::to_string(property_id) + " 6000 7000 24";
        assert(game_action("104", "600", "auction", property_listing.c_str(), 4000018,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("104", "601", "auction", "buyout 2", 4000019,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        for (int cycle = 0; cycle < 4; ++cycle) {
            assert(game_action("104", "602", "card", "charge 10", 4000020 + cycle,
                               game_output, sizeof(game_output)) == ECONOMY_OK);
            assert(game_action("104", "602", "repay", "10", 4000030 + cycle,
                               game_output, sizeof(game_output)) == ECONOMY_OK);
        }
        assert(game_action("104", "602", "property", "buy 1 mortgage", 4000040,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("mortgage autopay") != std::string::npos);
        assert(game_action("104", "600", "auction", "list relic 1 10 20 1", 4000041,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("104", "601", "auction", "bid 3 12", 4000042,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        EconomyPlayerSnapshot buyer_before_rent{};
        EconomyPlayerSnapshot buyer_after_rent{};
        assert(get_player("104", "601", &buyer_before_rent) == ECONOMY_OK);
        assert(tick_all(4000019 + 24 * 3600) == ECONOMY_OK);
        assert(get_player("104", "601", &buyer_after_rent) == ECONOMY_OK);
        assert(buyer_after_rent.wallet_cents > buyer_before_rent.wallet_cents);
        assert(game_action("104", "601", "collectible", "", 4086420,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Launch Relics:** 3") != std::string::npos);
        assert(game_action("104", "601", "collectible", "serials", 4086421,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("transfer(s)") != std::string::npos);
        assert(std::string(game_output).find("from <@600>") != std::string::npos);

        // Companies now have industries, production, equipment, demand,
        // dedicated financing, and escrowed player partnerships.
        assert(game_action("105", "999", "admin", "starting 100000", 5000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("105", "700", "business", "start tech", 5000001,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Technology") != std::string::npos);
        assert(game_action("105", "700", "business", "fund 5000", 5000002,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("105", "700", "supply", "buy 10", 5000003,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("105", "700", "produce", "10", 5000004,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("105", "700", "equipment", "upgrade", 5000005,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("105", "700", "marketing", "100", 5000006,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("105", "700", "businessloan", "borrow 1000", 5000007,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("105", "700", "partnership", "offer 701 20 1000", 5000008,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("105", "701", "partnership", "accept 1", 5000009,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        EconomyPlayerSnapshot partner_before_profit{};
        EconomyPlayerSnapshot partner_after_profit{};
        assert(get_player("105", "701", &partner_before_profit) == ECONOMY_OK);
        assert(game_action("105", "700", "business", "operate", 5000010,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Sold **10**") != std::string::npos);
        assert(get_player("105", "701", &partner_after_profit) == ECONOMY_OK);
        assert(partner_after_profit.wallet_cents > partner_before_profit.wallet_cents);
        assert(tick_all(5000010 + 24 * 3600) == ECONOMY_OK);

        // Three missed company-loan payments liquidate the company and move
        // the remaining liability into the owner's recoverable personal debt.
        assert(game_action("106", "999", "admin", "starting 100000", 6000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("106", "702", "business", "start retail", 6000001,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("106", "702", "businessloan", "borrow 1000", 6000002,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("106", "702", "business", "withdraw 2000", 6000003,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(tick_all(6000002 + 3 * 24 * 3600) == ECONOMY_OK);
        assert(game_action("106", "702", "businessloan", "", 6259203,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Defaults: **1**") != std::string::npos);

        // The exchange exposes fundamentals and finite market-maker depth.
        // Large market and limit orders therefore fill in deterministic slices.
        assert(game_action("107", "999", "admin", "starting 1000000", 7000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("107", "800", "market", "", 7000001,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Bid / ask") != std::string::npos);
        assert(game_action("107", "800", "fundamentals", "MEOW", 7000002,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Shares outstanding") != std::string::npos);
        assert(std::string(game_output).find("Estimated fundamental") != std::string::npos);
        assert(game_action("107", "800", "stock", "buy MEOW 500", 7000003,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("300/500") != std::string::npos);
        assert(std::string(game_output).find("partially filled") != std::string::npos);
        assert(game_action("107", "800", "orders", "place buy RAT 500 1000", 7000004,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(tick_all(7000004 + 3600) == ECONOMY_OK);
        assert(game_action("107", "800", "orders", "list", 7003605,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("#3") != std::string::npos);
        assert(std::string(game_output).find("×280") != std::string::npos);
        assert(game_action("107", "800", "enroll", "9", 7003606,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Psychology") != std::string::npos);
        assert(game_action("107", "800", "gamble", "blackjack 10", 7003607,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("blackjack") != std::string::npos);
        assert(game_action("107", "800", "certifications", "buy manager", 7003608,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Management Professional") != std::string::npos);
        assert(game_action("107", "800", "skills", "", 7003609,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Derived Skills") != std::string::npos);
        assert(game_action("107", "800", "bank", "", 7003610,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("stability") != std::string::npos);
        assert(tick_all(7007204) == ECONOMY_OK);
        assert(game_action("107", "800", "mystats", "", 7007205,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("Personal Economic History") != std::string::npos);
        assert(game_action("107", "800", "mychart", "networth", 7007206,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("hourly points") != std::string::npos);

        // Scheduled employment agreements execute daily and persist independently
        // from one-off player-to-player contracts.
        assert(game_action("108", "999", "admin", "starting 100000", 8000000,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("108", "810", "business", "start retail", 8000001,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(game_action("108", "810", "agreement", "offer employment 811 10 2", 8000002,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("#1") != std::string::npos);
        assert(game_action("108", "811", "agreement", "accept 1", 8000003,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        EconomyPlayerSnapshot employee_before{};
        EconomyPlayerSnapshot employee_after{};
        assert(get_player("108", "811", &employee_before) == ECONOMY_OK);
        assert(tick_all(8000003 + 24 * 3600) == ECONOMY_OK);
        assert(get_player("108", "811", &employee_after) == ECONOMY_OK);
        assert(employee_after.wallet_cents > employee_before.wallet_cents);
        assert(game_action("108", "811", "agreement", "list", 8086404,
                           game_output, sizeof(game_output)) == ECONOMY_OK);
        assert(std::string(game_output).find("ACTIVE") != std::string::npos);

        // New-account transfer ceilings and per-minute action limits blunt the
        // simplest laundering and command-flooding abuse patterns.
        assert(transfer("109", "813", "814", 10000001) == ECONOMY_INVALID_ARGUMENT);
        for (int index = 0; index < 30; ++index) {
            assert(game_action("109", "812", "skills", "", 9000000,
                               game_output, sizeof(game_output)) == ECONOMY_OK);
        }
        assert(game_action("109", "812", "skills", "", 9000000,
                           game_output, sizeof(game_output)) == ECONOMY_COOLDOWN);
        assert(std::string(game_output).find("rate limit") != std::string::npos);
        shutdown();
    }

    // A truncated primary save must recover from the last complete atomic
    // backup rather than silently booting a fresh economy.
    std::filesystem::resize_file(database, 64, ec);
    assert(!ec);

    // Loading the shared library again proves state survives unload/restart.
    {
        Library library(argv[1]);
        auto initialize = reinterpret_cast<extension_init_func>(
            library.symbol(EXTENSION_INIT_NAME));
        auto shutdown = reinterpret_cast<extension_shutdown_func>(
            library.symbol(EXTENSION_SHUTDOWN_NAME));
        auto get_api = reinterpret_cast<extension_get_api_func>(
            library.symbol(EXTENSION_GET_API_NAME));
        ExtensionKernelBridge bridge{};
        bridge.log = test_log;
        bridge.get_uptime = test_uptime;
        bridge.allocate = test_allocate;
        bridge.deallocate = test_deallocate;
        assert(initialize(&bridge) == 0);
        auto get_player = extension_function<economy_get_player_func>(
            get_api(), "economy_get_player");
        auto game_action = extension_function<economy_game_action_func>(
            get_api(), "economy_game_action");
        EconomyPlayerSnapshot persisted{};
        assert(get_player("100", "201", &persisted) == ECONOMY_OK);
        assert(persisted.wallet_cents >= 0 && persisted.wallet_cents < 4025);
        assert(persisted.item_quantities[1] == 1);
        char profile[1900]{};
        assert(game_action("100", "200", "profile", "", 1003600,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Finance") != std::string::npos);
        assert(std::string(profile).find("Commercial") == std::string::npos);
        assert(game_action("100", "200", "contract", "list", 1003601,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("PAID") != std::string::npos);
        assert(game_action("100", "201", "casino", "status", 1003601,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("level 1") != std::string::npos);
        assert(game_action("100", "200", "orders", "list", 1003601,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Open Orders") != std::string::npos);
        assert(game_action("100", "200", "history", "", 1003601,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("insurance") != std::string::npos ||
               std::string(profile).find("contract") != std::string::npos);
        assert(game_action("107", "800", "certifications", "", 9003601,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Management Professional") != std::string::npos);
        assert(game_action("108", "811", "agreement", "list", 9003601,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("#1") != std::string::npos);
        assert(game_action("100", "200", "stipend", "10,20", 1003601,
                           profile, sizeof(profile)) == ECONOMY_COOLDOWN);
        assert(game_action("103", "400", "government", "", 3086400,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Pro-Business") != std::string::npos);
        assert(game_action("103", "401", "crime", "status", 3086401,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Offenses: **1**") != std::string::npos);
        assert(game_action("103", "400", "chart", "market 2", 3090000,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("hourly points") != std::string::npos);
        assert(game_action("104", "601", "property", "", 4086420,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Apartment") != std::string::npos);
        assert(std::string(profile).find("Equity") != std::string::npos);
        assert(game_action("104", "601", "collectible", "", 4086421,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Launch Relics:** 3") != std::string::npos);
        assert(game_action("104", "601", "collectible", "serials", 4086422,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Serialized Launch Relics") != std::string::npos);
        assert(game_action("104", "602", "property", "", 4086422,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("mortgage") != std::string::npos);
        assert(game_action("105", "700", "business", "", 5086420,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Technology business") != std::string::npos);
        assert(std::string(profile).find("Equipment: **1/5") != std::string::npos);
        assert(game_action("105", "701", "partnership", "", 5086421,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("[ACTIVE]") != std::string::npos);
        assert(game_action("105", "700", "businessloan", "", 5086422,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Company financing") != std::string::npos);
        assert(game_action("106", "702", "businessloan", "", 6259204,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Defaults: **1**") != std::string::npos);
        assert(game_action("107", "800", "fundamentals", "RAT", 7003606,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Rolling volume") != std::string::npos);
        assert(game_action("107", "800", "orders", "list", 7003607,
                           profile, sizeof(profile)) == ECONOMY_OK);
        assert(std::string(profile).find("Open Orders") != std::string::npos);
        shutdown();
    }

    std::filesystem::remove(database, ec);
    std::filesystem::remove(database.string() + ".tmp", ec);
    std::filesystem::remove(database.string() + ".previous", ec);
    std::cout << "economy extension tests passed\n";
    return 0;
}
