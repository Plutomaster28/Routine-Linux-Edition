#include "../include/economy_extension_api.h"
#include "../include/module_interface.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace {

const KernelBridge* g_bridge = nullptr;
void* g_bot_context = nullptr;

economy_get_player_func g_get_player = nullptr;
economy_claim_func g_claim_daily = nullptr;
economy_claim_func g_work = nullptr;
economy_move_money_func g_move_money = nullptr;
economy_transfer_func g_transfer = nullptr;
economy_buy_item_func g_buy_item = nullptr;
economy_leaderboard_func g_leaderboard = nullptr;
economy_game_action_func g_game_action = nullptr;
economy_tick_all_func g_tick_all = nullptr;
economy_get_currency_func g_get_currency = nullptr;
thread_local std::string g_currency_symbol = "$";

struct ShopItem {
    const char* slug;
    const char* name;
    const char* description;
    int64_t price_cents;
};

constexpr std::array<ShopItem, ECONOMY_ITEM_COUNT> kShop = {{
    {"ramen", "Emergency Ramen", "A crunchy monument to liquidity problems.", 1250},
    {"coffee", "Questionable Coffee", "Productivity, anxiety, and a paper cup.", 475},
    {"calculator", "Suspicious Calculator", "It says every investment is a good idea.", 8500}
}};

void log_message(const char* level, const std::string& message) {
    if (g_bridge && g_bridge->log) g_bridge->log(level, message.c_str());
}

void reply(const char* channel_id, const std::string& message) {
    if (g_bridge && g_bridge->send_message) {
        g_bridge->send_message(g_bot_context, channel_id, message.c_str());
    }
}

const char* guild_for(const char* channel_id) {
    if (!g_bridge || !g_bridge->get_guild_id) return nullptr;
    return g_bridge->get_guild_id(g_bot_context, channel_id);
}

const char* require_guild(const char* channel_id) {
    const char* guild_id = guild_for(channel_id);
    if (!guild_id || !*guild_id) {
        reply(channel_id, "This economy lives inside a server. Economy commands are unavailable in DMs.");
        return nullptr;
    }
    if (g_get_currency) {
        std::array<char, 8> symbol{};
        std::array<char, 32> name{};
        if (g_get_currency(guild_id, symbol.data(), symbol.size(),
                           name.data(), name.size()) == ECONOMY_OK && symbol[0]) {
            g_currency_symbol = symbol.data();
        }
    }
    return guild_id;
}

std::string money(int64_t cents) {
    const bool negative = cents < 0;
    const uint64_t absolute = negative
        ? static_cast<uint64_t>(-(cents + 1)) + 1
        : static_cast<uint64_t>(cents);
    std::string whole = std::to_string(absolute / 100);
    for (int position = static_cast<int>(whole.size()) - 3; position > 0; position -= 3) {
        whole.insert(static_cast<size_t>(position), ",");
    }
    std::ostringstream output;
    if (negative) output << '-';
    output << g_currency_symbol << whole << '.' << std::setw(2)
           << std::setfill('0') << absolute % 100;
    return output.str();
}

std::string duration(int64_t seconds) {
    seconds = std::max<int64_t>(0, seconds);
    const int64_t hours = seconds / 3600;
    const int64_t minutes = (seconds % 3600) / 60;
    const int64_t secs = seconds % 60;
    std::ostringstream output;
    if (hours) output << hours << "h ";
    if (minutes || hours) output << minutes << "m ";
    output << secs << 's';
    return output.str();
}

std::string trim(std::string value) {
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool parse_money(std::string text, int64_t& cents) {
    text = trim(text);
    if (!text.empty() && text.front() == '$') text.erase(text.begin());
    if (!g_currency_symbol.empty() && text.rfind(g_currency_symbol, 0) == 0) {
        text.erase(0, g_currency_symbol.size());
    }
    text.erase(std::remove(text.begin(), text.end(), ','), text.end());
    if (text.empty() || text.front() == '-' || text.front() == '+') return false;

    const size_t decimal = text.find('.');
    if (decimal != std::string::npos && text.find('.', decimal + 1) != std::string::npos) {
        return false;
    }
    std::string dollars = decimal == std::string::npos ? text : text.substr(0, decimal);
    std::string fraction = decimal == std::string::npos ? "" : text.substr(decimal + 1);
    if (dollars.empty()) dollars = "0";
    if (fraction.size() > 2) return false;
    while (fraction.size() < 2) fraction.push_back('0');

    const auto digits_only = [](const std::string& value) {
        return !value.empty() && std::all_of(value.begin(), value.end(),
            [](unsigned char ch) { return std::isdigit(ch); });
    };
    if (!digits_only(dollars) || !digits_only(fraction)) return false;

    try {
        const uint64_t whole = std::stoull(dollars);
        const uint64_t part = static_cast<uint64_t>(std::stoul(fraction));
        if (whole > static_cast<uint64_t>(std::numeric_limits<int64_t>::max() / 100)) {
            return false;
        }
        const uint64_t total = whole * 100 + part;
        if (total == 0 || total > 100000000000000ULL) return false;
        cents = static_cast<int64_t>(total);
        return true;
    } catch (...) {
        return false;
    }
}

std::string normalize_user_id(std::string value) {
    value = trim(value);
    if (value.size() >= 4 && value.front() == '<' && value.back() == '>') {
        if (value.compare(0, 3, "<@!") == 0) {
            value = value.substr(3, value.size() - 4);
        } else if (value.compare(0, 2, "<@") == 0) {
            value = value.substr(2, value.size() - 3);
        }
    }
    if (value.empty() || value.size() > 31 ||
        !std::all_of(value.begin(), value.end(),
            [](unsigned char ch) { return std::isdigit(ch); })) {
        return {};
    }
    return value;
}

std::string status_error(int status) {
    switch (status) {
        case ECONOMY_INSUFFICIENT_FUNDS:
            return "You do not have enough money for that particular financial disaster.";
        case ECONOMY_SELF_TRANSFER:
            return "Paying yourself is accounting theater, not income.";
        case ECONOMY_STORAGE_ERROR:
            return "The economy vault could not save that transaction. Try again shortly.";
        default:
            return "That transaction was rejected. Check the command and amount.";
    }
}

void cmd_economy(void*, const char* channel_id, const char*, const char*) {
    reply(channel_id,
        "# Routine Economy · Version 2\n"
        "Local financial catastrophes, now connected to a living world economy.\n\n"
        "**Get moving**\n"
        "`/profile` `/balance` `/daily` `/work` `/pay` `/history` `/leaderboard`\n"
        "`/profile input: global` — your cross-server identity\n\n"
        "**Banking & credit**\n"
        "`/bank` `/savings` `/hysa` `/cd` `/card` `/loan` `/repay` `/margin` `/bankruptcy`\n\n"
        "**Career & education**\n"
        "`/career` `/applyjob` `/college` `/enroll` `/certifications` `/skills` `/stipend`\n\n"
        "**The exchange**\n"
        "`/market` `/fundamentals` `/stock` `/orders` `/short` `/derivatives` `/invest` `/portfolio` `/bonds` `/forex`\n\n"
        "**Build an empire**\n"
        "`/business` `/supply` `/produce` `/equipment` `/marketing` `/businessloan` "
        "`/partnership` `/payroll` `/corporate` `/property` `/auction`\n\n"
        "**Questionable decisions**\n"
        "`/gamble` `/casino` `/insurance` `/contract` `/agreement` `/crime` `/bail`\n\n"
        "**Government & intelligence**\n"
        "`/news` `/economyinfo` `/government` `/election` `/taxes` `/welfare` "
        "`/economystats` `/chart` `/mystats` `/mychart` `/rank`\n\n"
        "**Server administration:** `/econadmin`\n\n"
        "Choose any slash command and leave **input** blank to see its syntax. "
        "Legacy players can replace `/` with `~`. Server behavior now shapes "
        "confidence, cycles, currencies, trade, and world news.");
}

void run_game_action(const char* action, const char* channel_id,
                     const char* user_id, const char* args) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    std::array<char, 1900> output{};
    const int status = g_game_action(guild_id, user_id, action, args ? args : "",
                                     static_cast<int64_t>(std::time(nullptr)),
                                     output.data(), output.size());
    if (output[0]) {
        reply(channel_id, output.data());
    } else if (status != ECONOMY_OK) {
        reply(channel_id, status_error(status));
    }
}

#define GAME_COMMAND(function_name, action_name) \
    void function_name(void*, const char* channel_id, const char* user_id, const char* args) { \
        run_game_action(action_name, channel_id, user_id, args); \
    }

GAME_COMMAND(cmd_profile, "profile")
GAME_COMMAND(cmd_history, "history")
GAME_COMMAND(cmd_forex, "forex")
GAME_COMMAND(cmd_bank, "bank")
GAME_COMMAND(cmd_savings, "savings")
GAME_COMMAND(cmd_hysa, "hysa")
GAME_COMMAND(cmd_cd, "cd")
GAME_COMMAND(cmd_loan, "loan")
GAME_COMMAND(cmd_card, "card")
GAME_COMMAND(cmd_repay, "repay")
GAME_COMMAND(cmd_margin, "margin")
GAME_COMMAND(cmd_bankruptcy, "bankruptcy")
GAME_COMMAND(cmd_career, "career")
GAME_COMMAND(cmd_applyjob, "applyjob")
GAME_COMMAND(cmd_college, "college")
GAME_COMMAND(cmd_enroll, "enroll")
GAME_COMMAND(cmd_certifications, "certifications")
GAME_COMMAND(cmd_skills, "skills")
GAME_COMMAND(cmd_market, "market")
GAME_COMMAND(cmd_fundamentals, "fundamentals")
GAME_COMMAND(cmd_stock, "stock")
GAME_COMMAND(cmd_orders, "orders")
GAME_COMMAND(cmd_short, "short")
GAME_COMMAND(cmd_derivatives, "derivatives")
GAME_COMMAND(cmd_invest, "invest")
GAME_COMMAND(cmd_portfolio, "portfolio")
GAME_COMMAND(cmd_bonds, "bonds")
GAME_COMMAND(cmd_business, "business")
GAME_COMMAND(cmd_supply, "supply")
GAME_COMMAND(cmd_produce, "produce")
GAME_COMMAND(cmd_equipment, "equipment")
GAME_COMMAND(cmd_marketing, "marketing")
GAME_COMMAND(cmd_businessloan, "businessloan")
GAME_COMMAND(cmd_partnership, "partnership")
GAME_COMMAND(cmd_payroll, "payroll")
GAME_COMMAND(cmd_corporate, "corporate")
GAME_COMMAND(cmd_gamble, "gamble")
GAME_COMMAND(cmd_casino, "casino")
GAME_COMMAND(cmd_property, "property")
GAME_COMMAND(cmd_auction, "auction")
GAME_COMMAND(cmd_collectible, "collectible")
GAME_COMMAND(cmd_insurance, "insurance")
GAME_COMMAND(cmd_contract, "contract")
GAME_COMMAND(cmd_agreement, "agreement")
GAME_COMMAND(cmd_news, "news")
GAME_COMMAND(cmd_economyinfo, "economyinfo")
GAME_COMMAND(cmd_crime, "crime")
GAME_COMMAND(cmd_bail, "bail")
GAME_COMMAND(cmd_government, "government")
GAME_COMMAND(cmd_election, "election")
GAME_COMMAND(cmd_taxes, "taxes")
GAME_COMMAND(cmd_welfare, "welfare")
GAME_COMMAND(cmd_economystats, "economystats")
GAME_COMMAND(cmd_chart, "chart")
GAME_COMMAND(cmd_mystats, "mystats")
GAME_COMMAND(cmd_mychart, "mychart")
GAME_COMMAND(cmd_rank, "rank")

#undef GAME_COMMAND

void cmd_stipend(void*, const char* channel_id, const char* user_id, const char*) {
    const char* roles = g_bridge && g_bridge->get_user_roles
        ? g_bridge->get_user_roles(g_bot_context, channel_id, user_id) : "";
    run_game_action("stipend", channel_id, user_id, roles);
}

void cmd_econadmin(void*, const char* channel_id, const char* user_id, const char* args) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    if (!g_bridge || !g_bridge->is_guild_admin ||
        !g_bridge->is_guild_admin(g_bot_context, channel_id, user_id)) {
        reply(channel_id, "Only the server owner or a member with Administrator or Manage Server can change the economy.");
        return;
    }
    run_game_action("admin", channel_id, user_id, args);
}

void cmd_balance(void*, const char* channel_id, const char* user_id, const char*) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    EconomyPlayerSnapshot player{};
    const int status = g_get_player(guild_id, user_id, &player);
    if (status != ECONOMY_OK) {
        reply(channel_id, status_error(status));
        return;
    }
    reply(channel_id,
        "**Your Financial Situation**\n"
        "Wallet: **" + money(player.wallet_cents) + "**\n"
        "Checking: **" + money(player.checking_cents) + "**\n"
        "Net worth: **" + money(player.wallet_cents + player.checking_cents) + "**\n"
        "Shifts survived: **" + std::to_string(player.work_count) + "**");
}

void run_claim(const char* channel_id, const char* user_id, bool daily) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    int64_t award = 0;
    int64_t remaining = 0;
    const int status = (daily ? g_claim_daily : g_work)(
        guild_id, user_id, static_cast<int64_t>(std::time(nullptr)), &award, &remaining);
    if (status == ECONOMY_COOLDOWN) {
        reply(channel_id, std::string(daily ? "Your next daily bailout arrives in **"
                                           : "Labor regulations require a break. Work again in **") +
                          duration(remaining) + "**.");
        return;
    }
    if (status != ECONOMY_OK) {
        reply(channel_id, status_error(status));
        return;
    }
    reply(channel_id, std::string(daily ? "The treasury misplaced **" : "You completed a shift and earned **") +
                      money(award) + std::string(daily ? "** into your wallet."
                                                       : "**. Capitalism continues."));
}

void cmd_daily(void*, const char* channel_id, const char* user_id, const char*) {
    run_claim(channel_id, user_id, true);
}

void cmd_work(void*, const char* channel_id, const char* user_id, const char*) {
    run_claim(channel_id, user_id, false);
}

void move_money(const char* channel_id, const char* user_id, const char* args, bool deposit) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    int64_t amount = 0;
    if (!parse_money(args ? args : "", amount)) {
        reply(channel_id, std::string("Usage: `~") + (deposit ? "deposit" : "withdraw") +
                          " <amount>` — example: `~" + (deposit ? "deposit" : "withdraw") +
                          " 25.50`");
        return;
    }
    const int status = g_move_money(guild_id, user_id, amount, deposit ? 1 : 0);
    if (status != ECONOMY_OK) {
        reply(channel_id, status_error(status));
        return;
    }
    reply(channel_id, (deposit ? "Deposited **" : "Withdrew **") + money(amount) +
                      (deposit ? "** into checking." : "** into your wallet."));
}

void cmd_deposit(void*, const char* channel_id, const char* user_id, const char* args) {
    move_money(channel_id, user_id, args, true);
}

void cmd_withdraw(void*, const char* channel_id, const char* user_id, const char* args) {
    move_money(channel_id, user_id, args, false);
}

void cmd_pay(void*, const char* channel_id, const char* user_id, const char* args) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    std::istringstream input(args ? args : "");
    std::string recipient_text;
    std::string amount_text;
    input >> recipient_text >> amount_text;
    const std::string recipient = normalize_user_id(recipient_text);
    int64_t amount = 0;
    if (recipient.empty() || !parse_money(amount_text, amount)) {
        reply(channel_id, "Usage: `~pay <@user|user_id> <amount>`");
        return;
    }
    const int status = g_transfer(guild_id, user_id, recipient.c_str(), amount);
    if (status != ECONOMY_OK) {
        reply(channel_id, status_error(status));
        return;
    }
    reply(channel_id, "Transferred **" + money(amount) + "** to <@" + recipient +
                      ">. The audit trail is immaculate.");
}

void cmd_shop(void*, const char* channel_id, const char*, const char*) {
    if (!require_guild(channel_id)) return;
    std::ostringstream output;
    output << "**Miyamii Municipal Shop**\n";
    for (const ShopItem& item : kShop) {
        output << "`" << item.slug << "` — **" << money(item.price_cents) << "**\n"
               << item.description << '\n';
    }
    output << "Buy with `~buy <item> [quantity]`.";
    reply(channel_id, output.str());
}

void cmd_buy(void*, const char* channel_id, const char* user_id, const char* args) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    std::istringstream input(args ? args : "");
    std::string slug;
    uint32_t quantity = 1;
    input >> slug;
    if (input >> quantity) {
        std::string extra;
        if (input >> extra) quantity = 0;
    }
    std::transform(slug.begin(), slug.end(), slug.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    const auto item = std::find_if(kShop.begin(), kShop.end(),
        [&slug](const ShopItem& candidate) { return slug == candidate.slug; });
    if (item == kShop.end() || quantity == 0 || quantity > 100) {
        reply(channel_id, "Usage: `~buy <ramen|coffee|calculator> [quantity 1-100]`");
        return;
    }
    const uint32_t index = static_cast<uint32_t>(std::distance(kShop.begin(), item));
    const int status = g_buy_item(guild_id, user_id, index, quantity, item->price_cents);
    if (status != ECONOMY_OK) {
        reply(channel_id, status_error(status));
        return;
    }
    reply(channel_id, "Purchased **" + std::to_string(quantity) + "× " + item->name +
                      "** for **" + money(item->price_cents * quantity) + "**.");
}

void cmd_inventory(void*, const char* channel_id, const char* user_id, const char*) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    EconomyPlayerSnapshot player{};
    const int status = g_get_player(guild_id, user_id, &player);
    if (status != ECONOMY_OK) {
        reply(channel_id, status_error(status));
        return;
    }
    std::ostringstream output;
    output << "**Your Questionable Possessions**\n";
    bool empty = true;
    for (size_t i = 0; i < kShop.size(); ++i) {
        if (player.item_quantities[i] > 0) {
            empty = false;
            output << kShop[i].name << ": **" << player.item_quantities[i] << "**\n";
        }
    }
    if (empty) output << "An inspiring amount of nothing.";
    reply(channel_id, output.str());
}

void cmd_leaderboard(void*, const char* channel_id, const char*, const char*) {
    const char* guild_id = require_guild(channel_id);
    if (!guild_id) return;
    EconomyLeaderboardEntry entries[ECONOMY_MAX_LEADERBOARD]{};
    const size_t count = g_leaderboard(guild_id, entries, ECONOMY_MAX_LEADERBOARD);
    if (count == 0) {
        reply(channel_id, "No fortunes exist here yet. Be the first accounting error.");
        return;
    }
    std::ostringstream output;
    output << "**Server Net-Worth Leaderboard**\n";
    for (size_t i = 0; i < count; ++i) {
        output << (i + 1) << ". <@" << entries[i].user_id << "> — **"
               << money(entries[i].net_worth_cents) << "**\n";
    }
    reply(channel_id, output.str());
}

CommandRegistration g_commands[] = {
    {"economy", "Show local economy commands", cmd_economy},
    {"balance", "Show your wallet, checking, and net worth", cmd_balance},
    {"forex", "Exchange currencies between connected server economies", cmd_forex},
    {"daily", "Claim daily income", cmd_daily},
    {"work", "Work an odd job for income", cmd_work},
    {"deposit", "Move wallet money into checking", cmd_deposit},
    {"withdraw", "Move checking money into your wallet", cmd_withdraw},
    {"pay", "Transfer wallet money to another player", cmd_pay},
    {"shop", "Browse basic items", cmd_shop},
    {"buy", "Buy a basic item", cmd_buy},
    {"inventory", "Show your basic items", cmd_inventory},
    {"leaderboard", "Rank server players by net worth", cmd_leaderboard},
    {"profile", "Show complete finances, career, credit, and assets", cmd_profile},
    {"history", "Show your recent persistent financial history", cmd_history},
    {"bank", "Show banks, accounts, debt, and credit", cmd_bank},
    {"savings", "Deposit to or withdraw from savings", cmd_savings},
    {"hysa", "Deposit to or withdraw from high-yield savings", cmd_hysa},
    {"cd", "Open or close a certificate of deposit", cmd_cd},
    {"loan", "Apply for a credit-priced personal loan", cmd_loan},
    {"card", "Use a cash-back credit card", cmd_card},
    {"repay", "Repay debt and improve credit", cmd_repay},
    {"margin", "Borrow or repay margin debt", cmd_margin},
    {"bankruptcy", "Discharge debt by surrendering assets and credit", cmd_bankruptcy},
    {"career", "Show jobs and career requirements", cmd_career},
    {"applyjob", "Apply for a job tier", cmd_applyjob},
    {"college", "Show colleges, degrees, and tuition", cmd_college},
    {"enroll", "Buy a degree and unlock careers", cmd_enroll},
    {"certifications", "Earn specialized professional credentials", cmd_certifications},
    {"skills", "Inspect derived financial and professional skills", cmd_skills},
    {"stipend", "Claim income derived from real Discord roles", cmd_stipend},
    {"market", "Show the living local stock exchange", cmd_market},
    {"fundamentals", "Inspect a company, quote, liquidity, and valuation", cmd_fundamentals},
    {"stock", "Buy or sell local stocks", cmd_stock},
    {"orders", "Place, list, or cancel persistent limit orders", cmd_orders},
    {"short", "Open or cover short stock positions", cmd_short},
    {"derivatives", "Trade extreme-risk calls and puts", cmd_derivatives},
    {"invest", "Use retirement, index, and commodity investments", cmd_invest},
    {"portfolio", "Value stocks, bonds, business, and property", cmd_portfolio},
    {"bonds", "Buy or redeem government bonds", cmd_bonds},
    {"business", "Start, operate, and expand a company", cmd_business},
    {"supply", "Purchase industry-priced production inputs", cmd_supply},
    {"produce", "Convert raw materials into finished goods", cmd_produce},
    {"equipment", "Upgrade company production equipment", cmd_equipment},
    {"marketing", "Fund decaying customer-demand momentum", cmd_marketing},
    {"businessloan", "Borrow or repay dedicated company debt", cmd_businessloan},
    {"partnership", "Offer and manage player company equity", cmd_partnership},
    {"payroll", "Hire players and purchase business inventory", cmd_payroll},
    {"corporate", "Take a company public and acquire listed firms", cmd_corporate},
    {"gamble", "Play slots, blackjack, or roulette", cmd_gamble},
    {"casino", "Start and operate a player-owned casino", cmd_casino},
    {"property", "Browse and buy local property", cmd_property},
    {"auction", "List and bid on escrowed player assets", cmd_auction},
    {"collectible", "Trade serialized launch collectibles", cmd_collectible},
    {"insurance", "Buy asset coverage and file claims", cmd_insurance},
    {"contract", "Offer, accept, list, and repay player loans", cmd_contract},
    {"agreement", "Manage validated employment and rental schedules", cmd_agreement},
    {"news", "Read market-moving headlines", cmd_news},
    {"economyinfo", "Show inflation, unemployment, and confidence", cmd_economyinfo},
    {"crime", "Commit crimes or inspect your criminal record", cmd_crime},
    {"bail", "Pay the guild treasury to leave jail early", cmd_bail},
    {"government", "Inspect the mayor, policy, taxes, and treasury", cmd_government},
    {"election", "Run for mayor, vote, or inspect the election", cmd_election},
    {"taxes", "Inspect policy or make a treasury payment", cmd_taxes},
    {"welfare", "Claim treasury-funded unemployment assistance", cmd_welfare},
    {"economystats", "Inspect measured guild economic aggregates", cmd_economystats},
    {"chart", "Chart hourly economy or stock history", cmd_chart},
    {"mystats", "Inspect your rolling personal financial history", cmd_mystats},
    {"mychart", "Chart your net worth, cash, debt, or investments", cmd_mychart},
    {"rank", "Rank players across financial categories", cmd_rank},
    {"econadmin", "Configure this server's economy (server admins)", cmd_econadmin},
    {nullptr, nullptr, nullptr}
};

template <typename Function>
bool load_function(Function& target, const char* name) {
    target = reinterpret_cast<Function>(
        g_bridge->get_extension_function(g_bot_context, name));
    if (!target) log_message("ERROR", std::string("Missing economy extension function: ") + name);
    return target != nullptr;
}

}  // namespace

extern "C" {

ModuleInfo module_get_info() {
    return {
        "local_economy_game",
        "2.0.0",
        "Routine Team",
        "Persistent local economies connected by a global simulation",
        MODULE_API_VERSION,
        MODULE_TYPE_NATIVE
    };
}

int module_init(const KernelBridge* bridge, void* bot_context) {
    g_bridge = bridge;
    g_bot_context = bot_context;
    if (!g_bridge || !g_bridge->get_extension_function || !g_bridge->get_guild_id ||
        !g_bridge->get_user_roles || !g_bridge->is_guild_admin) {
        log_message("ERROR", "Economy game requires Routine module API v4");
        return 1;
    }

    auto api_version = reinterpret_cast<economy_api_version_func>(
        g_bridge->get_extension_function(g_bot_context, "economy_api_version"));
    if (!api_version || api_version() != ECONOMY_EXTENSION_API_VERSION) {
        log_message("ERROR", "Compatible local_economy extension is not loaded");
        return 2;
    }

    const bool loaded =
        load_function(g_get_player, "economy_get_player") &&
        load_function(g_claim_daily, "economy_claim_daily") &&
        load_function(g_work, "economy_work") &&
        load_function(g_move_money, "economy_move_money") &&
        load_function(g_transfer, "economy_transfer") &&
        load_function(g_buy_item, "economy_buy_item") &&
        load_function(g_leaderboard, "economy_leaderboard") &&
        load_function(g_game_action, "economy_game_action") &&
        load_function(g_get_currency, "economy_get_currency");
    if (loaded && !load_function(g_tick_all, "economy_tick_all")) return 3;
    if (!loaded) return 3;
    log_message("INFO", "Local economy game initialized");
    return 0;
}

void module_shutdown() {
    log_message("INFO", "Local economy game shutting down");
    g_bridge = nullptr;
    g_bot_context = nullptr;
    g_get_player = nullptr;
    g_claim_daily = nullptr;
    g_work = nullptr;
    g_move_money = nullptr;
    g_transfer = nullptr;
    g_buy_item = nullptr;
    g_leaderboard = nullptr;
    g_game_action = nullptr;
    g_tick_all = nullptr;
    g_get_currency = nullptr;
    g_currency_symbol = "$";
}

const CommandRegistration* module_register_commands() {
    return g_commands;
}

void module_on_tick(void*) {
    static int64_t last_tick = 0;
    const int64_t now = static_cast<int64_t>(std::time(nullptr));
    if (g_tick_all && now - last_tick >= 60) {
        g_tick_all(now);
        last_tick = now;
    }
}

}
