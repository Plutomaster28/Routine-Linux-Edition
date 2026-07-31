#include "../include/economy_extension_api.h"
#include "../include/extension_interface.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>
#include <limits>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct Player {
    int64_t wallet_cents = 5000;
    int64_t checking_cents = 0;
    int64_t last_daily_unix = 0;
    int64_t last_work_unix = 0;
    uint64_t work_count = 0;
    std::array<uint32_t, ECONOMY_ITEM_COUNT> items{};
};

constexpr size_t kStockCount = 9;

struct ChaosPlayer {
    int64_t savings_cents = 0;
    int64_t hysa_cents = 0;
    int64_t debt_cents = 0;
    int64_t bonds_cents = 0;
    int64_t business_cash_cents = 0;
    int64_t last_business_unix = 0;
    int32_t credit_score = 600;
    uint32_t experience = 0;
    uint32_t job_tier = 0;
    uint32_t degree = 0;
    uint32_t business_level = 0;
    uint32_t property_level = 0;
    uint32_t bankruptcies = 0;
    uint64_t gambling_wagered_cents = 0;
    int64_t gambling_profit_cents = 0;
    std::array<uint32_t, kStockCount> shares{};
};

struct GuildEconomy {
    std::array<int64_t, kStockCount> prices{
        4200, 1800, 3200, 2700, 6660, 3900, 2400, 8500, 500
    };
    int64_t last_market_unix = 0;
    int32_t inflation_bp = 250;
    int32_t unemployment_bp = 550;
    int32_t confidence = 55;
    uint32_t event_id = 0;
};

struct GuildExchange {
    std::array<int64_t, kStockCount> revenue{
        5200000, 2100000, 3900000, 4300000, 6800000,
        3600000, 4700000, 7500000, 900000
    };
    std::array<int64_t, kStockCount> expenses{
        3900000, 1700000, 3100000, 3700000, 5100000,
        2800000, 4000000, 5400000, 1050000
    };
    std::array<int64_t, kStockCount> profit{};
    std::array<int64_t, kStockCount> cash{
        8000000000LL, 2100000000LL, 3500000000LL, 6200000000LL,
        5000000000LL, 4100000000LL, 3300000000LL, 12000000000LL, 600000000LL
    };
    std::array<int64_t, kStockCount> debt{
        900000000LL, 1200000000LL, 700000000LL, 1800000000LL,
        4800000000LL, 1300000000LL, 2200000000LL, 900000000LL, 1100000000LL
    };
    std::array<uint64_t, kStockCount> shares_outstanding{
        1500000, 1200000, 1800000, 1600000, 2000000,
        1400000, 1700000, 1300000, 2500000
    };
    std::array<uint64_t, kStockCount> volume{};
    std::array<uint32_t, kStockCount> liquidity{
        300, 220, 500, 260, 140, 350, 240, 400, 90
    };
    std::array<uint32_t, kStockCount> remaining_liquidity{
        300, 220, 500, 260, 140, 350, 240, 400, 90
    };
    std::array<int32_t, kStockCount> sentiment{
        65, 48, 55, 50, 42, 58, 47, 70, 60
    };
    std::array<int64_t, kStockCount> halted_until{};
    std::array<uint32_t, kStockCount> distress{};
    std::array<bool, kStockCount> listed{
        true, true, true, true, true, true, true, true, true
    };
};

struct AdvancedPlayer {
    int64_t cd_cents = 0;
    int64_t cd_opened_unix = 0;
    int64_t retirement_cents = 0;
    int64_t index_fund_cents = 0;
    int64_t commodities_cents = 0;
    int64_t margin_debt_cents = 0;
    int64_t casino_reserve_cents = 0;
    int64_t casino_profit_cents = 0;
    int64_t last_casino_unix = 0;
    int64_t last_insurance_claim_unix = 0;
    uint32_t insurance_level = 0;
    uint32_t collectibles = 0;
    uint32_t casino_level = 0;
    uint32_t business_inventory = 0;
    uint32_t employees = 0;
    std::array<uint32_t, kStockCount> short_shares{};
};

struct PlayerContract {
    uint64_t id = 0;
    std::string guild_id;
    std::string lender_id;
    std::string borrower_id;
    int64_t principal_cents = 0;
    int64_t repayment_cents = 0;
    bool accepted = false;
    bool repaid = false;
};

struct LimitOrder {
    uint64_t id = 0;
    std::string guild_id;
    std::string user_id;
    uint32_t stock = 0;
    uint32_t quantity = 0;
    int64_t limit_price_cents = 0;
    bool buy = false;
};

struct AuditEntry {
    std::string guild_id;
    std::string user_id;
    int64_t timestamp = 0;
    std::string action;
    std::string summary;
};

struct CorporatePlayer {
    int64_t last_stipend_unix = 0;
    uint32_t acquisitions = 0;
    bool public_company = false;
};

struct GuildSettings {
    std::string currency_name = "Dollar";
    std::string currency_symbol = "$";
    int64_t starting_balance_cents = 5000;
    int64_t role_stipend_cents = 500;
};

struct FinancialLifecycle {
    int64_t next_payment_unix = 0;
    int64_t last_review_unix = 0;
    uint32_t bank_id = 0;
    uint32_t missed_payments = 0;
    uint32_t defaults = 0;
    int32_t performance = 50;
    uint32_t layoffs = 0;
};

struct CrimePlayer {
    int64_t jailed_until_unix = 0;
    int64_t last_crime_unix = 0;
    int64_t last_welfare_unix = 0;
    int64_t last_heat_decay_unix = 0;
    int64_t fines_paid_cents = 0;
    int64_t taxes_paid_cents = 0;
    uint32_t heat = 0;
    uint32_t offenses = 0;
    uint32_t successes = 0;
};

struct GovernmentState {
    std::string mayor_id;
    int64_t treasury_cents = 50000;
    int64_t election_end_unix = 0;
    int64_t term_end_unix = 0;
    int32_t tax_basis_points = 500;
    int64_t welfare_cents = 2500;
    uint32_t platform = 0;
};

struct ElectionCandidate {
    std::string guild_id;
    std::string user_id;
    uint32_t platform = 0;
};

struct EconomySnapshot {
    std::string guild_id;
    int64_t timestamp = 0;
    int64_t money_supply_cents = 0;
    int64_t average_net_worth_cents = 0;
    int64_t median_net_worth_cents = 0;
    int64_t total_debt_cents = 0;
    int64_t business_value_cents = 0;
    int64_t investment_value_cents = 0;
    uint32_t active_players = 0;
    uint32_t employed_players = 0;
    uint32_t item_supply = 0;
    int32_t bank_stability = 100;
    std::array<int64_t, kStockCount> stock_prices{};
};

struct PropertyAsset {
    uint64_t id = 0;
    std::string guild_id;
    std::string owner_id;
    uint32_t tier = 0;
    int64_t purchase_price_cents = 0;
    int64_t market_value_cents = 0;
    int64_t mortgage_cents = 0;
    int64_t next_payment_unix = 0;
    int64_t last_income_unix = 0;
    uint32_t condition = 100;
    uint32_t missed_payments = 0;
    bool listed = false;
};

struct AuctionListing {
    uint64_t id = 0;
    std::string guild_id;
    std::string seller_id;
    uint32_t asset_type = 0;  // 0: collectible stack, 1: property
    uint64_t asset_id = 0;
    uint32_t quantity = 0;
    int64_t reserve_cents = 0;
    int64_t buyout_cents = 0;
    int64_t highest_bid_cents = 0;
    std::string highest_bidder_id;
    int64_t end_unix = 0;
};

struct BusinessProfile {
    uint32_t industry = 0;
    uint32_t equipment_level = 0;
    uint32_t marketing = 0;
    int32_t reputation = 50;
    uint32_t raw_materials = 0;
    uint32_t finished_goods = 0;
    int64_t debt_cents = 0;
    int64_t next_payment_unix = 0;
    int64_t last_decay_unix = 0;
    int64_t lifetime_revenue_cents = 0;
    int64_t lifetime_profit_cents = 0;
    uint32_t missed_payments = 0;
    uint32_t defaults = 0;
};

struct BusinessPartnership {
    uint64_t id = 0;
    std::string guild_id;
    std::string owner_id;
    std::string partner_id;
    uint32_t share_basis_points = 0;
    int64_t contribution_cents = 0;
    bool accepted = false;
};

struct CollectibleAsset {
    uint64_t serial = 0;
    std::string guild_id;
    std::string owner_id;
    std::string previous_owner_id;
    int64_t minted_unix = 0;
    int64_t acquired_unix = 0;
    uint64_t auction_id = 0;
    uint32_t transfers = 0;
    uint32_t rarity = 0;
};

struct PlayerDevelopment {
    uint32_t certifications = 0;
};

struct PlayerHistoryPoint {
    std::string guild_id;
    std::string user_id;
    int64_t timestamp = 0;
    int64_t net_worth_cents = 0;
    int64_t liquid_cents = 0;
    int64_t debt_cents = 0;
    int64_t invested_cents = 0;
};

struct GuildBankNetwork {
    std::array<int32_t, 6> stability{72, 61, 80, 88, 68, 48};
    std::array<int64_t, 6> failed_until{};
    int64_t last_update_unix = 0;
};

struct SecurityProfile {
    int64_t created_unix = 0;
    int64_t action_window_unix = 0;
    uint32_t actions_in_window = 0;
    uint32_t rejected_actions = 0;
};

struct ScheduledAgreement {
    uint64_t id = 0;
    std::string guild_id;
    std::string issuer_id;
    std::string counterparty_id;
    uint32_t type = 0;  // 0 employment, 1 rental
    uint64_t asset_id = 0;
    int64_t payment_cents = 0;
    uint32_t remaining_payments = 0;
    int64_t next_payment_unix = 0;
    bool accepted = false;
    bool active = false;
};

struct GlobalPlayer {
    uint32_t education_mask = 0;
    uint32_t licenses = 0;
    uint64_t achievements = 0;
    int32_t reputation = 50;
    uint64_t lifetime_actions = 0;
    uint64_t lifetime_trade_cents = 0;
    uint64_t lifetime_forex_cents = 0;
    int64_t first_seen_unix = 0;
    int64_t last_seen_unix = 0;
};

struct GuildDynamics {
    int64_t last_cycle_unix = 0;
    int64_t spending_cents = 0;
    int64_t investment_cents = 0;
    int64_t saving_cents = 0;
    int64_t selling_cents = 0;
    int64_t capital_inflow_cents = 0;
    int64_t capital_outflow_cents = 0;
    int64_t exports_cents = 0;
    int64_t imports_cents = 0;
    int64_t government_debt_cents = 0;
    int64_t forex_volume_cents = 0;
    int64_t last_stimulus_unix = 0;
    int32_t policy_rate_bp = 400;
    int32_t currency_index = 10000;
    int32_t trend = 0;
    uint32_t hires = 0;
    uint32_t layoffs = 0;
    uint32_t recession_hours = 0;
    uint32_t recovery_hours = 0;
    uint32_t personality = 0;
    uint64_t last_global_event_id = 0;
    int32_t tariff_basis_points = 200;
    std::string trade_partner;
    std::array<int32_t, kStockCount> expectations{};
    std::array<int64_t, kStockCount> rumor_due{};
};

struct NewsEvent {
    uint64_t id = 0;
    std::string origin_guild;
    int64_t created_unix = 0;
    int64_t evolves_unix = 0;
    int32_t company = -1;
    int32_t impact = 0;
    uint32_t rarity = 0;
    uint32_t stage = 0;
    bool global = false;
    bool positive = true;
    std::string headline;
};

const ExtensionKernelBridge* g_kernel = nullptr;
std::mutex g_mutex;
std::unordered_map<std::string, Player> g_players;
std::unordered_map<std::string, ChaosPlayer> g_chaos_players;
std::unordered_map<std::string, AdvancedPlayer> g_advanced_players;
std::unordered_map<std::string, GuildEconomy> g_guilds;
std::unordered_map<std::string, GuildExchange> g_exchanges;
std::vector<PlayerContract> g_contracts;
std::vector<LimitOrder> g_orders;
std::vector<AuditEntry> g_audit;
std::unordered_map<std::string, CorporatePlayer> g_corporate_players;
std::unordered_map<std::string, GuildSettings> g_settings;
std::unordered_map<std::string, FinancialLifecycle> g_lifecycle;
std::unordered_map<std::string, CrimePlayer> g_crime_players;
std::unordered_map<std::string, GovernmentState> g_governments;
std::vector<ElectionCandidate> g_candidates;
std::unordered_map<std::string, std::string> g_votes;
std::vector<EconomySnapshot> g_snapshots;
std::vector<PropertyAsset> g_properties;
std::vector<AuctionListing> g_auctions;
std::unordered_map<std::string, BusinessProfile> g_business_profiles;
std::vector<BusinessPartnership> g_partnerships;
std::vector<CollectibleAsset> g_collectible_assets;
std::unordered_map<std::string, PlayerDevelopment> g_development;
std::vector<PlayerHistoryPoint> g_player_history;
std::unordered_map<std::string, GuildBankNetwork> g_bank_networks;
std::unordered_map<std::string, SecurityProfile> g_security;
std::vector<ScheduledAgreement> g_agreements;
std::unordered_map<std::string, GlobalPlayer> g_global_players;
std::unordered_map<std::string, GuildDynamics> g_dynamics;
std::vector<NewsEvent> g_news;
uint64_t g_next_contract_id = 1;
uint64_t g_next_order_id = 1;
uint64_t g_next_property_id = 1;
uint64_t g_next_auction_id = 1;
uint64_t g_next_partnership_id = 1;
uint64_t g_next_collectible_serial = 1;
uint64_t g_next_agreement_id = 1;
uint64_t g_next_news_id = 1;
std::mt19937_64 g_rng{std::random_device{}()};
bool g_loaded = false;
bool g_loaded_from_previous = false;
uint32_t g_data_migration_version = 0;
thread_local std::string g_display_symbol = "$";

constexpr int64_t kDailyCooldown = 24 * 60 * 60;
constexpr int64_t kWorkCooldown = 60 * 60;
constexpr int64_t kMaximumTransaction = 100000000000000LL;

void add_bounded(int64_t& target, int64_t amount);
GlobalPlayer& touch_global_player(const std::string& user_id, int64_t now);
GlobalPlayer& sync_global_player(const std::string& user_id,
                                 ChaosPlayer& local,
                                 PlayerDevelopment& development,
                                 int64_t now);

std::string key_for(const std::string& guild_id, const std::string& user_id) {
    return guild_id + '\x1f' + user_id;
}

Player& ensure_player(const std::string& guild_id, const std::string& user_id) {
    const std::string key = key_for(guild_id, user_id);
    auto [it, inserted] = g_players.try_emplace(key);
    if (inserted) {
        it->second.wallet_cents = g_settings[guild_id].starting_balance_cents;
    }
    return it->second;
}

bool has_managed_property(const std::string& guild_id, const std::string& user_id) {
    return std::any_of(g_properties.begin(), g_properties.end(),
        [&](const PropertyAsset& property) {
            return property.guild_id == guild_id && property.owner_id == user_id;
        });
}

fs::path data_path() {
    const char* override_path = std::getenv("ROUTINE_ECONOMY_DATA");
    return override_path && *override_path
        ? fs::path(override_path)
        : fs::path("data") / "local_economy_v1.db";
}

void log_message(const std::string& message) {
    if (g_kernel && g_kernel->log) {
        g_kernel->log(message.c_str());
    }
}

bool valid_id(const char* value) {
    if (!value || !*value) return false;
    size_t length = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value); *p; ++p) {
        if (*p < '0' || *p > '9' || ++length > 31) return false;
    }
    return length > 0;
}

bool complete_database_file(const fs::path& path) {
    std::ifstream input(path);
    if (!input) return false;
    std::string magic;
    unsigned version = 0;
    if (!(input >> magic >> version) || magic != "ROUTINE_ECONOMY" ||
        (version != 1 && version != 2 && version != 3)) return false;
    if (version == 1) return true;
    std::string token;
    std::string last;
    while (input >> token) last = token;
    return last == "END";
}

bool state_invariants_hold() {
    constexpr size_t kMaximumPlayers = 100000;
    constexpr size_t kMaximumOpenRecords = 100000;
    if (g_players.size() > kMaximumPlayers ||
        g_orders.size() > kMaximumOpenRecords ||
        g_contracts.size() > kMaximumOpenRecords ||
        g_auctions.size() > kMaximumOpenRecords ||
        g_agreements.size() > kMaximumOpenRecords ||
        g_properties.size() > kMaximumOpenRecords ||
        g_collectible_assets.size() > 1000000 ||
        g_player_history.size() > 100000 ||
        g_global_players.size() > kMaximumPlayers ||
        g_news.size() > 1000) {
        return false;
    }
    for (const auto& entry : g_players) {
        if (entry.second.wallet_cents < 0 || entry.second.checking_cents < 0 ||
            entry.second.wallet_cents > kMaximumTransaction ||
            entry.second.checking_cents > kMaximumTransaction) return false;
    }
    for (const auto& entry : g_chaos_players) {
        const ChaosPlayer& player = entry.second;
        if (player.savings_cents < 0 || player.hysa_cents < 0 ||
            player.debt_cents < 0 || player.bonds_cents < 0 ||
            player.business_cash_cents < 0 ||
            player.credit_score < 300 || player.credit_score > 850) return false;
    }
    for (const auto& entry : g_advanced_players) {
        const AdvancedPlayer& player = entry.second;
        if (player.cd_cents < 0 || player.retirement_cents < 0 ||
            player.index_fund_cents < 0 || player.commodities_cents < 0 ||
            player.margin_debt_cents < 0 || player.casino_reserve_cents < 0) {
            return false;
        }
    }
    for (const LimitOrder& order : g_orders) {
        if (!order.quantity || order.stock >= kStockCount ||
            order.limit_price_cents <= 0 ||
            order.limit_price_cents > kMaximumTransaction / order.quantity) {
            return false;
        }
    }
    for (const auto& entry : g_dynamics) {
        const GuildDynamics& dynamics = entry.second;
        if (dynamics.policy_rate_bp < 0 || dynamics.policy_rate_bp > 5000 ||
            dynamics.currency_index < 1000 || dynamics.currency_index > 50000 ||
            dynamics.tariff_basis_points < 0 ||
            dynamics.tariff_basis_points > 2500) {
            return false;
        }
    }
    return true;
}

bool save_locked() {
    if (!state_invariants_hold()) {
        log_message("[ECONOMY] Refusing to persist state that violates invariants");
        return false;
    }
    const fs::path path = data_path();
    std::error_code ec;
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path(), ec);
        if (ec) return false;
    }

    fs::path temporary = path;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::trunc);
    if (!output) return false;

    output << "ROUTINE_ECONOMY 3\n";
    output << "MV " << g_data_migration_version << '\n';
    for (const auto& entry : g_players) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const std::string guild = entry.first.substr(0, split);
        const std::string user = entry.first.substr(split + 1);
        const Player& player = entry.second;
        output << "P " << std::quoted(guild) << ' ' << std::quoted(user) << ' '
               << player.wallet_cents << ' ' << player.checking_cents << ' '
               << player.last_daily_unix << ' ' << player.last_work_unix << ' '
               << player.work_count;
        for (uint32_t quantity : player.items) output << ' ' << quantity;
        output << '\n';
    }
    for (const auto& entry : g_chaos_players) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const ChaosPlayer& player = entry.second;
        output << "C " << std::quoted(entry.first.substr(0, split)) << ' '
               << std::quoted(entry.first.substr(split + 1)) << ' '
               << player.savings_cents << ' ' << player.hysa_cents << ' '
               << player.debt_cents << ' ' << player.bonds_cents << ' '
               << player.business_cash_cents << ' ' << player.last_business_unix << ' '
               << player.credit_score << ' ' << player.experience << ' '
               << player.job_tier << ' ' << player.degree << ' '
               << player.business_level << ' ' << player.property_level << ' '
               << player.bankruptcies << ' ' << player.gambling_wagered_cents << ' '
               << player.gambling_profit_cents;
        for (uint32_t shares : player.shares) output << ' ' << shares;
        output << '\n';
    }
    for (const auto& entry : g_guilds) {
        const GuildEconomy& guild = entry.second;
        output << "G " << std::quoted(entry.first) << ' ' << guild.last_market_unix << ' '
               << guild.inflation_bp << ' ' << guild.unemployment_bp << ' '
               << guild.confidence << ' ' << guild.event_id;
        for (int64_t price : guild.prices) output << ' ' << price;
        output << '\n';
    }
    for (const auto& entry : g_exchanges) {
        const GuildExchange& exchange = entry.second;
        output << "F " << std::quoted(entry.first);
        for (int64_t value : exchange.revenue) output << ' ' << value;
        for (int64_t value : exchange.expenses) output << ' ' << value;
        for (int64_t value : exchange.profit) output << ' ' << value;
        for (int64_t value : exchange.cash) output << ' ' << value;
        for (int64_t value : exchange.debt) output << ' ' << value;
        for (uint64_t value : exchange.shares_outstanding) output << ' ' << value;
        for (uint64_t value : exchange.volume) output << ' ' << value;
        for (uint32_t value : exchange.liquidity) output << ' ' << value;
        for (uint32_t value : exchange.remaining_liquidity) output << ' ' << value;
        for (int32_t value : exchange.sentiment) output << ' ' << value;
        for (int64_t value : exchange.halted_until) output << ' ' << value;
        for (uint32_t value : exchange.distress) output << ' ' << value;
        for (bool value : exchange.listed) output << ' ' << value;
        output << '\n';
    }
    for (const auto& entry : g_advanced_players) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const AdvancedPlayer& player = entry.second;
        output << "A " << std::quoted(entry.first.substr(0, split)) << ' '
               << std::quoted(entry.first.substr(split + 1)) << ' '
               << player.cd_cents << ' ' << player.cd_opened_unix << ' '
               << player.retirement_cents << ' ' << player.index_fund_cents << ' '
               << player.commodities_cents << ' ' << player.margin_debt_cents << ' '
               << player.casino_reserve_cents << ' ' << player.casino_profit_cents << ' '
               << player.last_casino_unix << ' ' << player.last_insurance_claim_unix << ' '
               << player.insurance_level << ' ' << player.collectibles << ' '
               << player.casino_level << ' ' << player.business_inventory << ' '
               << player.employees;
        for (uint32_t shares : player.short_shares) output << ' ' << shares;
        output << '\n';
    }
    for (const PlayerContract& contract : g_contracts) {
        output << "R " << contract.id << ' ' << std::quoted(contract.guild_id) << ' '
               << std::quoted(contract.lender_id) << ' '
               << std::quoted(contract.borrower_id) << ' '
               << contract.principal_cents << ' ' << contract.repayment_cents << ' '
               << contract.accepted << ' ' << contract.repaid << '\n';
    }
    for (const LimitOrder& order : g_orders) {
        output << "O " << order.id << ' ' << std::quoted(order.guild_id) << ' '
               << std::quoted(order.user_id) << ' ' << order.stock << ' '
               << order.quantity << ' ' << order.limit_price_cents << ' '
               << order.buy << '\n';
    }
    for (const AuditEntry& audit : g_audit) {
        output << "H " << std::quoted(audit.guild_id) << ' '
               << std::quoted(audit.user_id) << ' ' << audit.timestamp << ' '
               << std::quoted(audit.action) << ' ' << std::quoted(audit.summary) << '\n';
    }
    for (const auto& entry : g_corporate_players) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        output << "X " << std::quoted(entry.first.substr(0, split)) << ' '
               << std::quoted(entry.first.substr(split + 1)) << ' '
               << entry.second.last_stipend_unix << ' '
               << entry.second.acquisitions << ' '
               << entry.second.public_company << '\n';
    }
    for (const auto& entry : g_settings) {
        output << "S " << std::quoted(entry.first) << ' '
               << std::quoted(entry.second.currency_name) << ' '
               << std::quoted(entry.second.currency_symbol) << ' '
               << entry.second.starting_balance_cents << ' '
               << entry.second.role_stipend_cents << '\n';
    }
    for (const auto& entry : g_lifecycle) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const FinancialLifecycle& lifecycle = entry.second;
        output << "L " << std::quoted(entry.first.substr(0, split)) << ' '
               << std::quoted(entry.first.substr(split + 1)) << ' '
               << lifecycle.next_payment_unix << ' ' << lifecycle.last_review_unix << ' '
               << lifecycle.bank_id << ' ' << lifecycle.missed_payments << ' '
               << lifecycle.defaults << ' ' << lifecycle.performance << ' '
               << lifecycle.layoffs << '\n';
    }
    for (const auto& entry : g_crime_players) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const CrimePlayer& crime = entry.second;
        output << "K " << std::quoted(entry.first.substr(0, split)) << ' '
               << std::quoted(entry.first.substr(split + 1)) << ' '
               << crime.jailed_until_unix << ' ' << crime.last_crime_unix << ' '
               << crime.last_welfare_unix << ' ' << crime.last_heat_decay_unix << ' '
               << crime.fines_paid_cents << ' ' << crime.taxes_paid_cents << ' '
               << crime.heat << ' ' << crime.offenses << ' ' << crime.successes << '\n';
    }
    for (const auto& entry : g_governments) {
        const GovernmentState& government = entry.second;
        output << "Q " << std::quoted(entry.first) << ' '
               << std::quoted(government.mayor_id) << ' '
               << government.treasury_cents << ' ' << government.election_end_unix << ' '
               << government.term_end_unix << ' ' << government.tax_basis_points << ' '
               << government.welfare_cents << ' ' << government.platform << '\n';
    }
    for (const ElectionCandidate& candidate : g_candidates) {
        output << "E " << std::quoted(candidate.guild_id) << ' '
               << std::quoted(candidate.user_id) << ' ' << candidate.platform << '\n';
    }
    for (const auto& vote : g_votes) {
        const size_t split = vote.first.find('\x1f');
        if (split == std::string::npos) continue;
        output << "V " << std::quoted(vote.first.substr(0, split)) << ' '
               << std::quoted(vote.first.substr(split + 1)) << ' '
               << std::quoted(vote.second) << '\n';
    }
    for (const EconomySnapshot& snapshot : g_snapshots) {
        output << "T " << std::quoted(snapshot.guild_id) << ' '
               << snapshot.timestamp << ' ' << snapshot.money_supply_cents << ' '
               << snapshot.average_net_worth_cents << ' '
               << snapshot.median_net_worth_cents << ' '
               << snapshot.total_debt_cents << ' ' << snapshot.business_value_cents << ' '
               << snapshot.investment_value_cents << ' ' << snapshot.active_players << ' '
               << snapshot.employed_players << ' ' << snapshot.item_supply << ' '
               << snapshot.bank_stability;
        for (int64_t price : snapshot.stock_prices) output << ' ' << price;
        output << '\n';
    }
    for (const PropertyAsset& property : g_properties) {
        output << "U " << property.id << ' ' << std::quoted(property.guild_id) << ' '
               << std::quoted(property.owner_id) << ' ' << property.tier << ' '
               << property.purchase_price_cents << ' ' << property.market_value_cents << ' '
               << property.mortgage_cents << ' ' << property.next_payment_unix << ' '
               << property.last_income_unix << ' ' << property.condition << ' '
               << property.missed_payments << ' ' << property.listed << '\n';
    }
    for (const AuctionListing& auction : g_auctions) {
        output << "Z " << auction.id << ' ' << std::quoted(auction.guild_id) << ' '
               << std::quoted(auction.seller_id) << ' ' << auction.asset_type << ' '
               << auction.asset_id << ' ' << auction.quantity << ' '
               << auction.reserve_cents << ' ' << auction.buyout_cents << ' '
               << auction.highest_bid_cents << ' '
               << std::quoted(auction.highest_bidder_id) << ' '
               << auction.end_unix << '\n';
    }
    for (const auto& entry : g_business_profiles) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const BusinessProfile& business = entry.second;
        output << "B " << std::quoted(entry.first.substr(0, split)) << ' '
               << std::quoted(entry.first.substr(split + 1)) << ' '
               << business.industry << ' ' << business.equipment_level << ' '
               << business.marketing << ' ' << business.reputation << ' '
               << business.raw_materials << ' ' << business.finished_goods << ' '
               << business.debt_cents << ' ' << business.next_payment_unix << ' '
               << business.last_decay_unix << ' ' << business.lifetime_revenue_cents << ' '
               << business.lifetime_profit_cents << ' ' << business.missed_payments << ' '
               << business.defaults << '\n';
    }
    for (const BusinessPartnership& partnership : g_partnerships) {
        output << "J " << partnership.id << ' '
               << std::quoted(partnership.guild_id) << ' '
               << std::quoted(partnership.owner_id) << ' '
               << std::quoted(partnership.partner_id) << ' '
               << partnership.share_basis_points << ' '
               << partnership.contribution_cents << ' '
               << partnership.accepted << '\n';
    }
    for (const CollectibleAsset& collectible : g_collectible_assets) {
        output << "N " << collectible.serial << ' '
               << std::quoted(collectible.guild_id) << ' '
               << std::quoted(collectible.owner_id) << ' '
               << std::quoted(collectible.previous_owner_id) << ' '
               << collectible.minted_unix << ' ' << collectible.acquired_unix << ' '
               << collectible.auction_id << ' ' << collectible.transfers << ' '
               << collectible.rarity << '\n';
    }
    for (const auto& entry : g_development) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        output << "D " << std::quoted(entry.first.substr(0, split)) << ' '
               << std::quoted(entry.first.substr(split + 1)) << ' '
               << entry.second.certifications << '\n';
    }
    for (const PlayerHistoryPoint& point : g_player_history) {
        output << "W " << std::quoted(point.guild_id) << ' '
               << std::quoted(point.user_id) << ' ' << point.timestamp << ' '
               << point.net_worth_cents << ' ' << point.liquid_cents << ' '
               << point.debt_cents << ' ' << point.invested_cents << '\n';
    }
    for (const auto& entry : g_bank_networks) {
        output << "Y " << std::quoted(entry.first) << ' '
               << entry.second.last_update_unix;
        for (int32_t value : entry.second.stability) output << ' ' << value;
        for (int64_t value : entry.second.failed_until) output << ' ' << value;
        output << '\n';
    }
    for (const auto& entry : g_security) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        output << "I " << std::quoted(entry.first.substr(0, split)) << ' '
               << std::quoted(entry.first.substr(split + 1)) << ' '
               << entry.second.created_unix << ' '
               << entry.second.action_window_unix << ' '
               << entry.second.actions_in_window << ' '
               << entry.second.rejected_actions << '\n';
    }
    for (const ScheduledAgreement& agreement : g_agreements) {
        output << "R2 " << agreement.id << ' '
               << std::quoted(agreement.guild_id) << ' '
               << std::quoted(agreement.issuer_id) << ' '
               << std::quoted(agreement.counterparty_id) << ' '
               << agreement.type << ' ' << agreement.asset_id << ' '
               << agreement.payment_cents << ' '
               << agreement.remaining_payments << ' '
               << agreement.next_payment_unix << ' '
               << agreement.accepted << ' ' << agreement.active << '\n';
    }
    for (const auto& entry : g_global_players) {
        const GlobalPlayer& player = entry.second;
        output << "GP " << std::quoted(entry.first) << ' '
               << player.education_mask << ' ' << player.licenses << ' '
               << player.achievements << ' ' << player.reputation << ' '
               << player.lifetime_actions << ' '
               << player.lifetime_trade_cents << ' '
               << player.lifetime_forex_cents << ' '
               << player.first_seen_unix << ' ' << player.last_seen_unix << '\n';
    }
    for (const auto& entry : g_dynamics) {
        const GuildDynamics& dynamics = entry.second;
        output << "M " << std::quoted(entry.first) << ' '
               << dynamics.last_cycle_unix << ' '
               << dynamics.spending_cents << ' '
               << dynamics.investment_cents << ' '
               << dynamics.saving_cents << ' '
               << dynamics.selling_cents << ' '
               << dynamics.capital_inflow_cents << ' '
               << dynamics.capital_outflow_cents << ' '
               << dynamics.exports_cents << ' '
               << dynamics.imports_cents << ' '
               << dynamics.government_debt_cents << ' '
               << dynamics.forex_volume_cents << ' '
               << dynamics.last_stimulus_unix << ' '
               << dynamics.policy_rate_bp << ' '
               << dynamics.currency_index << ' '
               << dynamics.trend << ' '
               << dynamics.hires << ' ' << dynamics.layoffs << ' '
               << dynamics.recession_hours << ' '
               << dynamics.recovery_hours << ' '
               << dynamics.personality << ' '
               << dynamics.last_global_event_id << ' '
               << dynamics.tariff_basis_points << ' '
               << std::quoted(dynamics.trade_partner);
        for (int32_t value : dynamics.expectations) output << ' ' << value;
        for (int64_t value : dynamics.rumor_due) output << ' ' << value;
        output << '\n';
    }
    for (const NewsEvent& event : g_news) {
        output << "NE " << event.id << ' '
               << std::quoted(event.origin_guild) << ' '
               << event.created_unix << ' ' << event.evolves_unix << ' '
               << event.company << ' ' << event.impact << ' '
               << event.rarity << ' ' << event.stage << ' '
               << event.global << ' ' << event.positive << ' '
               << std::quoted(event.headline) << '\n';
    }
    output << "END\n";
    output.flush();
    if (!output) return false;
    output.close();

    if (fs::exists(path) && !g_loaded_from_previous) {
        fs::path previous = path;
        previous += ".previous";
        std::error_code backup_copy_error;
        fs::copy_file(path, previous, fs::copy_options::overwrite_existing,
                      backup_copy_error);
    }

    fs::rename(temporary, path, ec);
    if (!ec) {
        g_loaded_from_previous = false;
        return true;
    }

    // Windows does not replace an existing destination during rename. Keep a
    // recoverable backup until the replacement is safely in place.
    fs::path backup = path;
    backup += ".bak";
    std::error_code backup_error;
    fs::remove(backup, backup_error);
    backup_error.clear();
    fs::rename(path, backup, backup_error);
    if (backup_error) return false;

    ec.clear();
    fs::rename(temporary, path, ec);
    if (ec) {
        std::error_code restore_error;
        fs::rename(backup, path, restore_error);
        return false;
    }
    fs::remove(backup, backup_error);
    g_loaded_from_previous = false;
    return true;
}

bool migrate_beta_global_players_locked(const fs::path& source_path) {
    constexpr uint32_t kBestOfMigrationVersion = 1;
    if (g_data_migration_version >= kBestOfMigrationVersion) return true;

    fs::path archive = data_path();
    archive += ".pre_global_merge";
    std::error_code ec;
    if (!fs::exists(archive)) {
        fs::copy_file(source_path, archive, fs::copy_options::none, ec);
        if (ec) {
            log_message("[ECONOMY] Beta migration paused: could not create the "
                        "pre-global-merge archive");
            return false;
        }
    }

    const auto globals_before = g_global_players;
    const uint32_t migration_before = g_data_migration_version;
    std::unordered_map<std::string, uint32_t> server_counts;
    std::unordered_map<std::string, uint64_t> best_audit_counts;
    std::unordered_map<std::string, uint64_t> per_server_audit_counts;

    for (const auto& entry : g_audit) {
        ++per_server_audit_counts[key_for(entry.guild_id, entry.user_id)];
    }
    for (const auto& entry : per_server_audit_counts) {
        const size_t split = entry.first.find('\x1f');
        if (split != std::string::npos) {
            const std::string user = entry.first.substr(split + 1);
            best_audit_counts[user] =
                std::max(best_audit_counts[user], entry.second);
        }
    }

    for (const auto& entry : g_players) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const std::string user = entry.first.substr(split + 1);
        GlobalPlayer& global = g_global_players[user];
        ++server_counts[user];
        global.lifetime_actions = std::max<uint64_t>(
            global.lifetime_actions,
            std::max(entry.second.work_count, best_audit_counts[user]));
    }

    for (const auto& entry : g_chaos_players) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const std::string user = entry.first.substr(split + 1);
        const ChaosPlayer& local = entry.second;
        GlobalPlayer& global = g_global_players[user];
        if (local.degree > 0 && local.degree <= 9) {
            global.education_mask |= 1u << local.degree;
        }
        global.lifetime_actions = std::max<uint64_t>(
            global.lifetime_actions, local.experience);
        int32_t legacy_reputation =
            35 + (local.credit_score - 300) * 35 / 550 +
            static_cast<int32_t>(local.degree) * 2;
        const auto business = g_business_profiles.find(entry.first);
        if (business != g_business_profiles.end()) {
            legacy_reputation += business->second.reputation / 5;
        }
        const auto crime = g_crime_players.find(entry.first);
        if (crime != g_crime_players.end()) {
            legacy_reputation -= static_cast<int32_t>(
                std::min<uint32_t>(20, crime->second.offenses));
        }
        global.reputation =
            std::max(global.reputation, std::clamp(legacy_reputation, 0, 100));
    }

    for (const auto& entry : g_development) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        GlobalPlayer& global = g_global_players[entry.first.substr(split + 1)];
        global.licenses |= entry.second.certifications;
    }

    for (const auto& entry : g_security) {
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos || entry.second.created_unix <= 0) continue;
        GlobalPlayer& global = g_global_players[entry.first.substr(split + 1)];
        if (!global.first_seen_unix) {
            global.first_seen_unix = entry.second.created_unix;
        } else {
            global.first_seen_unix =
                std::min(global.first_seen_unix, entry.second.created_unix);
        }
        global.last_seen_unix =
            std::max(global.last_seen_unix, entry.second.created_unix);
    }

    for (auto& entry : g_global_players) {
        GlobalPlayer& global = entry.second;
        if (global.lifetime_actions) global.achievements |= 1ull << 0;
        if (server_counts[entry.first] >= 2) global.achievements |= 1ull << 1;
        if (global.education_mask) global.achievements |= 1ull << 2;
        if (global.licenses) global.achievements |= 1ull << 3;
    }

    g_data_migration_version = kBestOfMigrationVersion;
    if (!save_locked()) {
        g_global_players = globals_before;
        g_data_migration_version = migration_before;
        log_message("[ECONOMY] Beta best-of migration could not be saved; "
                    "local state was left unchanged");
        return false;
    }
    log_message("[ECONOMY] Beta data migrated with best-of global progression "
                "rules; server balances remain separate. Archive: " +
                archive.string());
    return true;
}

void load_locked() {
    if (g_loaded) return;
    g_loaded = true;

    fs::path load_path = data_path();
    if (fs::exists(load_path) && !complete_database_file(load_path)) {
        fs::path previous = load_path;
        previous += ".previous";
        if (complete_database_file(previous)) {
            log_message("[ECONOMY] Primary database incomplete; loading previous snapshot");
            load_path = previous;
            g_loaded_from_previous = true;
        }
    }
    std::ifstream input(load_path);
    if (!input) {
        g_data_migration_version = 1;
        return;
    }

    std::string magic;
    unsigned version = 0;
    if (!(input >> magic >> version) || magic != "ROUTINE_ECONOMY" ||
        (version != 1 && version != 2 && version != 3)) {
        log_message("[ECONOMY] Ignoring unsupported or corrupt data file");
        return;
    }

    std::string record;
    while (input >> record) {
        if (record == "END") break;
        if (record == "MV") {
            if (!(input >> g_data_migration_version)) break;
            continue;
        }
        if (record == "C") {
            std::string guild;
            std::string user;
            ChaosPlayer player;
            if (!(input >> std::quoted(guild) >> std::quoted(user)
                  >> player.savings_cents >> player.hysa_cents >> player.debt_cents
                  >> player.bonds_cents >> player.business_cash_cents
                  >> player.last_business_unix >> player.credit_score
                  >> player.experience >> player.job_tier >> player.degree
                  >> player.business_level >> player.property_level
                  >> player.bankruptcies >> player.gambling_wagered_cents
                  >> player.gambling_profit_cents)) break;
            bool failed = false;
            for (uint32_t& shares : player.shares) {
                if (!(input >> shares)) { failed = true; break; }
            }
            if (failed) break;
            if (!guild.empty() && !user.empty()) {
                g_chaos_players[key_for(guild, user)] = player;
            }
            continue;
        }
        if (record == "G") {
            std::string guild_id;
            GuildEconomy guild;
            if (!(input >> std::quoted(guild_id) >> guild.last_market_unix
                  >> guild.inflation_bp >> guild.unemployment_bp
                  >> guild.confidence >> guild.event_id)) break;
            bool failed = false;
            for (int64_t& price : guild.prices) {
                if (!(input >> price)) { failed = true; break; }
            }
            if (failed) break;
            if (!guild_id.empty()) g_guilds[guild_id] = guild;
            continue;
        }
        if (record == "F") {
            std::string guild_id;
            GuildExchange exchange;
            if (!(input >> std::quoted(guild_id))) break;
            bool failed = false;
#define READ_EXCHANGE_ARRAY(field) \
            for (auto& value : exchange.field) { \
                if (!(input >> value)) { failed = true; break; } \
            } \
            if (failed) break
            READ_EXCHANGE_ARRAY(revenue);
            READ_EXCHANGE_ARRAY(expenses);
            READ_EXCHANGE_ARRAY(profit);
            READ_EXCHANGE_ARRAY(cash);
            READ_EXCHANGE_ARRAY(debt);
            READ_EXCHANGE_ARRAY(shares_outstanding);
            READ_EXCHANGE_ARRAY(volume);
            READ_EXCHANGE_ARRAY(liquidity);
            READ_EXCHANGE_ARRAY(remaining_liquidity);
            READ_EXCHANGE_ARRAY(sentiment);
            READ_EXCHANGE_ARRAY(halted_until);
            READ_EXCHANGE_ARRAY(distress);
            READ_EXCHANGE_ARRAY(listed);
#undef READ_EXCHANGE_ARRAY
            if (!guild_id.empty()) {
                for (size_t i = 0; i < kStockCount; ++i) {
                    exchange.shares_outstanding[i] =
                        std::max<uint64_t>(1, exchange.shares_outstanding[i]);
                    exchange.liquidity[i] =
                        std::clamp<uint32_t>(exchange.liquidity[i], 1, 1000000);
                    exchange.remaining_liquidity[i] =
                        std::min(exchange.remaining_liquidity[i],
                                 exchange.liquidity[i]);
                    exchange.sentiment[i] =
                        std::clamp(exchange.sentiment[i], 0, 100);
                }
                g_exchanges[guild_id] = exchange;
            }
            continue;
        }
        if (record == "A") {
            std::string guild;
            std::string user;
            AdvancedPlayer player;
            if (!(input >> std::quoted(guild) >> std::quoted(user)
                  >> player.cd_cents >> player.cd_opened_unix
                  >> player.retirement_cents >> player.index_fund_cents
                  >> player.commodities_cents >> player.margin_debt_cents
                  >> player.casino_reserve_cents >> player.casino_profit_cents
                  >> player.last_casino_unix >> player.last_insurance_claim_unix
                  >> player.insurance_level >> player.collectibles
                  >> player.casino_level >> player.business_inventory
                  >> player.employees)) break;
            bool failed = false;
            for (uint32_t& shares : player.short_shares) {
                if (!(input >> shares)) { failed = true; break; }
            }
            if (failed) break;
            if (!guild.empty() && !user.empty()) {
                g_advanced_players[key_for(guild, user)] = player;
            }
            continue;
        }
        if (record == "R") {
            PlayerContract contract;
            if (!(input >> contract.id >> std::quoted(contract.guild_id)
                  >> std::quoted(contract.lender_id) >> std::quoted(contract.borrower_id)
                  >> contract.principal_cents >> contract.repayment_cents
                  >> contract.accepted >> contract.repaid)) break;
            g_contracts.push_back(contract);
            g_next_contract_id = std::max(g_next_contract_id, contract.id + 1);
            continue;
        }
        if (record == "O") {
            LimitOrder order;
            if (!(input >> order.id >> std::quoted(order.guild_id)
                  >> std::quoted(order.user_id) >> order.stock >> order.quantity
                  >> order.limit_price_cents >> order.buy)) break;
            if (order.stock < kStockCount) {
                g_orders.push_back(order);
                g_next_order_id = std::max(g_next_order_id, order.id + 1);
            }
            continue;
        }
        if (record == "H") {
            AuditEntry audit;
            if (!(input >> std::quoted(audit.guild_id) >> std::quoted(audit.user_id)
                  >> audit.timestamp >> std::quoted(audit.action)
                  >> std::quoted(audit.summary))) break;
            g_audit.push_back(std::move(audit));
            continue;
        }
        if (record == "X") {
            std::string guild;
            std::string user;
            CorporatePlayer corporate;
            if (!(input >> std::quoted(guild) >> std::quoted(user)
                  >> corporate.last_stipend_unix >> corporate.acquisitions
                  >> corporate.public_company)) break;
            if (!guild.empty() && !user.empty()) {
                g_corporate_players[key_for(guild, user)] = corporate;
            }
            continue;
        }
        if (record == "S") {
            std::string guild;
            GuildSettings settings;
            if (!(input >> std::quoted(guild) >> std::quoted(settings.currency_name)
                  >> std::quoted(settings.currency_symbol)
                  >> settings.starting_balance_cents
                  >> settings.role_stipend_cents)) break;
            if (!guild.empty()) g_settings[guild] = settings;
            continue;
        }
        if (record == "L") {
            std::string guild;
            std::string user;
            FinancialLifecycle lifecycle;
            if (!(input >> std::quoted(guild) >> std::quoted(user)
                  >> lifecycle.next_payment_unix >> lifecycle.last_review_unix
                  >> lifecycle.bank_id >> lifecycle.missed_payments
                  >> lifecycle.defaults >> lifecycle.performance
                  >> lifecycle.layoffs)) break;
            if (!guild.empty() && !user.empty()) {
                g_lifecycle[key_for(guild, user)] = lifecycle;
            }
            continue;
        }
        if (record == "K") {
            std::string guild;
            std::string user;
            CrimePlayer crime;
            if (!(input >> std::quoted(guild) >> std::quoted(user)
                  >> crime.jailed_until_unix >> crime.last_crime_unix
                  >> crime.last_welfare_unix >> crime.last_heat_decay_unix
                  >> crime.fines_paid_cents >> crime.taxes_paid_cents
                  >> crime.heat >> crime.offenses >> crime.successes)) break;
            if (!guild.empty() && !user.empty()) {
                g_crime_players[key_for(guild, user)] = crime;
            }
            continue;
        }
        if (record == "Q") {
            std::string guild;
            GovernmentState government;
            if (!(input >> std::quoted(guild) >> std::quoted(government.mayor_id)
                  >> government.treasury_cents >> government.election_end_unix
                  >> government.term_end_unix >> government.tax_basis_points
                  >> government.welfare_cents >> government.platform)) break;
            if (!guild.empty()) g_governments[guild] = government;
            continue;
        }
        if (record == "E") {
            ElectionCandidate candidate;
            if (!(input >> std::quoted(candidate.guild_id)
                  >> std::quoted(candidate.user_id) >> candidate.platform)) break;
            if (!candidate.guild_id.empty() && !candidate.user_id.empty()) {
                g_candidates.push_back(std::move(candidate));
            }
            continue;
        }
        if (record == "V") {
            std::string guild;
            std::string voter;
            std::string candidate;
            if (!(input >> std::quoted(guild) >> std::quoted(voter)
                  >> std::quoted(candidate))) break;
            if (!guild.empty() && !voter.empty() && !candidate.empty()) {
                g_votes[key_for(guild, voter)] = candidate;
            }
            continue;
        }
        if (record == "T") {
            EconomySnapshot snapshot;
            if (!(input >> std::quoted(snapshot.guild_id) >> snapshot.timestamp
                  >> snapshot.money_supply_cents >> snapshot.average_net_worth_cents
                  >> snapshot.median_net_worth_cents >> snapshot.total_debt_cents
                  >> snapshot.business_value_cents >> snapshot.investment_value_cents
                  >> snapshot.active_players >> snapshot.employed_players
                  >> snapshot.item_supply >> snapshot.bank_stability)) break;
            bool failed = false;
            for (int64_t& price : snapshot.stock_prices) {
                if (!(input >> price)) { failed = true; break; }
            }
            if (failed) break;
            if (!snapshot.guild_id.empty()) g_snapshots.push_back(std::move(snapshot));
            continue;
        }
        if (record == "U") {
            PropertyAsset property;
            if (!(input >> property.id >> std::quoted(property.guild_id)
                  >> std::quoted(property.owner_id) >> property.tier
                  >> property.purchase_price_cents >> property.market_value_cents
                  >> property.mortgage_cents >> property.next_payment_unix
                  >> property.last_income_unix >> property.condition
                  >> property.missed_payments >> property.listed)) break;
            if (!property.guild_id.empty() && !property.owner_id.empty() &&
                property.tier >= 1 && property.tier <= 3) {
                g_properties.push_back(property);
                g_next_property_id = std::max(g_next_property_id, property.id + 1);
            }
            continue;
        }
        if (record == "Z") {
            AuctionListing auction;
            if (!(input >> auction.id >> std::quoted(auction.guild_id)
                  >> std::quoted(auction.seller_id) >> auction.asset_type
                  >> auction.asset_id >> auction.quantity >> auction.reserve_cents
                  >> auction.buyout_cents >> auction.highest_bid_cents
                  >> std::quoted(auction.highest_bidder_id) >> auction.end_unix)) break;
            if (!auction.guild_id.empty() && !auction.seller_id.empty() &&
                auction.asset_type <= 1) {
                g_auctions.push_back(auction);
                g_next_auction_id = std::max(g_next_auction_id, auction.id + 1);
            }
            continue;
        }
        if (record == "B") {
            std::string guild;
            std::string user;
            BusinessProfile business;
            if (!(input >> std::quoted(guild) >> std::quoted(user)
                  >> business.industry >> business.equipment_level
                  >> business.marketing >> business.reputation
                  >> business.raw_materials >> business.finished_goods
                  >> business.debt_cents >> business.next_payment_unix
                  >> business.last_decay_unix >> business.lifetime_revenue_cents
                  >> business.lifetime_profit_cents >> business.missed_payments
                  >> business.defaults)) break;
            if (!guild.empty() && !user.empty() && business.industry <= 5) {
                business.equipment_level =
                    std::min<uint32_t>(business.equipment_level, 5);
                business.marketing = std::min<uint32_t>(business.marketing, 100);
                business.reputation = std::clamp(business.reputation, 0, 100);
                g_business_profiles[key_for(guild, user)] = business;
            }
            continue;
        }
        if (record == "J") {
            BusinessPartnership partnership;
            if (!(input >> partnership.id >> std::quoted(partnership.guild_id)
                  >> std::quoted(partnership.owner_id)
                  >> std::quoted(partnership.partner_id)
                  >> partnership.share_basis_points
                  >> partnership.contribution_cents
                  >> partnership.accepted)) break;
            if (!partnership.guild_id.empty() && !partnership.owner_id.empty() &&
                !partnership.partner_id.empty() &&
                partnership.share_basis_points > 0 &&
                partnership.share_basis_points < 5000) {
                g_partnerships.push_back(partnership);
                g_next_partnership_id =
                    std::max(g_next_partnership_id, partnership.id + 1);
            }
            continue;
        }
        if (record == "N") {
            CollectibleAsset collectible;
            if (!(input >> collectible.serial
                  >> std::quoted(collectible.guild_id)
                  >> std::quoted(collectible.owner_id)
                  >> std::quoted(collectible.previous_owner_id)
                  >> collectible.minted_unix >> collectible.acquired_unix
                  >> collectible.auction_id >> collectible.transfers
                  >> collectible.rarity)) break;
            if (!collectible.guild_id.empty() && !collectible.owner_id.empty() &&
                collectible.rarity <= 3) {
                g_collectible_assets.push_back(collectible);
                g_next_collectible_serial =
                    std::max(g_next_collectible_serial, collectible.serial + 1);
            }
            continue;
        }
        if (record == "D") {
            std::string guild;
            std::string user;
            PlayerDevelopment development;
            if (!(input >> std::quoted(guild) >> std::quoted(user)
                  >> development.certifications)) break;
            development.certifications &= 0x1Fu;
            if (!guild.empty() && !user.empty()) {
                g_development[key_for(guild, user)] = development;
            }
            continue;
        }
        if (record == "W") {
            PlayerHistoryPoint point;
            if (!(input >> std::quoted(point.guild_id)
                  >> std::quoted(point.user_id) >> point.timestamp
                  >> point.net_worth_cents >> point.liquid_cents
                  >> point.debt_cents >> point.invested_cents)) break;
            if (!point.guild_id.empty() && !point.user_id.empty()) {
                g_player_history.push_back(std::move(point));
            }
            continue;
        }
        if (record == "Y") {
            std::string guild;
            GuildBankNetwork network;
            if (!(input >> std::quoted(guild) >> network.last_update_unix)) break;
            bool failed = false;
            for (int32_t& value : network.stability) {
                if (!(input >> value)) { failed = true; break; }
                value = std::clamp(value, 0, 100);
            }
            if (failed) break;
            for (int64_t& value : network.failed_until) {
                if (!(input >> value)) { failed = true; break; }
            }
            if (failed) break;
            if (!guild.empty()) g_bank_networks[guild] = network;
            continue;
        }
        if (record == "I") {
            std::string guild;
            std::string user;
            SecurityProfile security;
            if (!(input >> std::quoted(guild) >> std::quoted(user)
                  >> security.created_unix >> security.action_window_unix
                  >> security.actions_in_window
                  >> security.rejected_actions)) break;
            security.actions_in_window =
                std::min<uint32_t>(security.actions_in_window, 1000);
            if (!guild.empty() && !user.empty()) {
                g_security[key_for(guild, user)] = security;
            }
            continue;
        }
        if (record == "R2") {
            ScheduledAgreement agreement;
            if (!(input >> agreement.id >> std::quoted(agreement.guild_id)
                  >> std::quoted(agreement.issuer_id)
                  >> std::quoted(agreement.counterparty_id)
                  >> agreement.type >> agreement.asset_id
                  >> agreement.payment_cents >> agreement.remaining_payments
                  >> agreement.next_payment_unix
                  >> agreement.accepted >> agreement.active)) break;
            if (!agreement.guild_id.empty() && !agreement.issuer_id.empty() &&
                !agreement.counterparty_id.empty() && agreement.type <= 1 &&
                agreement.payment_cents > 0 &&
                agreement.remaining_payments <= 365) {
                g_agreements.push_back(agreement);
                g_next_agreement_id =
                    std::max(g_next_agreement_id, agreement.id + 1);
            }
            continue;
        }
        if (record == "GP") {
            std::string user;
            GlobalPlayer player;
            if (!(input >> std::quoted(user)
                  >> player.education_mask >> player.licenses
                  >> player.achievements >> player.reputation
                  >> player.lifetime_actions >> player.lifetime_trade_cents
                  >> player.lifetime_forex_cents
                  >> player.first_seen_unix >> player.last_seen_unix)) break;
            player.reputation = std::clamp(player.reputation, 0, 100);
            if (!user.empty()) g_global_players[user] = player;
            continue;
        }
        if (record == "M") {
            std::string guild;
            GuildDynamics dynamics;
            if (!(input >> std::quoted(guild)
                  >> dynamics.last_cycle_unix
                  >> dynamics.spending_cents
                  >> dynamics.investment_cents
                  >> dynamics.saving_cents
                  >> dynamics.selling_cents
                  >> dynamics.capital_inflow_cents
                  >> dynamics.capital_outflow_cents
                  >> dynamics.exports_cents >> dynamics.imports_cents
                  >> dynamics.government_debt_cents
                  >> dynamics.forex_volume_cents
                  >> dynamics.last_stimulus_unix
                  >> dynamics.policy_rate_bp
                  >> dynamics.currency_index >> dynamics.trend
                  >> dynamics.hires >> dynamics.layoffs
                  >> dynamics.recession_hours
                  >> dynamics.recovery_hours
                  >> dynamics.personality
                  >> dynamics.last_global_event_id
                  >> dynamics.tariff_basis_points
                  >> std::quoted(dynamics.trade_partner))) break;
            bool failed = false;
            for (int32_t& value : dynamics.expectations) {
                if (!(input >> value)) { failed = true; break; }
            }
            if (failed) break;
            for (int64_t& value : dynamics.rumor_due) {
                if (!(input >> value)) { failed = true; break; }
            }
            if (failed) break;
            dynamics.policy_rate_bp =
                std::clamp(dynamics.policy_rate_bp, 0, 5000);
            dynamics.currency_index =
                std::clamp(dynamics.currency_index, 1000, 50000);
            dynamics.tariff_basis_points =
                std::clamp(dynamics.tariff_basis_points, 0, 2500);
            if (!guild.empty()) g_dynamics[guild] = dynamics;
            continue;
        }
        if (record == "NE") {
            NewsEvent event;
            if (!(input >> event.id >> std::quoted(event.origin_guild)
                  >> event.created_unix >> event.evolves_unix
                  >> event.company >> event.impact
                  >> event.rarity >> event.stage
                  >> event.global >> event.positive
                  >> std::quoted(event.headline))) break;
            if (!event.origin_guild.empty() && event.company < int(kStockCount) &&
                event.rarity <= 3) {
                g_news.push_back(event);
                g_next_news_id = std::max(g_next_news_id, event.id + 1);
            }
            continue;
        }
        if (record != "P") {
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        std::string guild;
        std::string user;
        Player player;
        if (!(input >> std::quoted(guild) >> std::quoted(user)
              >> player.wallet_cents >> player.checking_cents
              >> player.last_daily_unix >> player.last_work_unix
              >> player.work_count)) {
            break;
        }
        bool item_error = false;
        for (uint32_t& quantity : player.items) {
            if (!(input >> quantity)) {
                item_error = true;
                break;
            }
        }
        if (item_error) break;
        if (!guild.empty() && !user.empty() && player.wallet_cents >= 0 &&
            player.checking_cents >= 0) {
            g_players[key_for(guild, user)] = player;
        }
    }
    migrate_beta_global_players_locked(load_path);
}

void snapshot(const Player& player, EconomyPlayerSnapshot* out) {
    out->wallet_cents = player.wallet_cents;
    out->checking_cents = player.checking_cents;
    out->last_daily_unix = player.last_daily_unix;
    out->last_work_unix = player.last_work_unix;
    out->work_count = player.work_count;
    for (size_t i = 0; i < player.items.size(); ++i) {
        out->item_quantities[i] = player.items[i];
    }
}

GlobalPlayer& touch_global_player(const std::string& user_id, int64_t now) {
    GlobalPlayer& global = g_global_players[user_id];
    if (now > 0) {
        if (!global.first_seen_unix) global.first_seen_unix = now;
        global.last_seen_unix = std::max(global.last_seen_unix, now);
    }
    return global;
}

uint32_t economy_api_version_impl() {
    return ECONOMY_EXTENSION_API_VERSION;
}

int economy_get_player_impl(const char* guild_id, const char* user_id,
                            EconomyPlayerSnapshot* out) {
    if (!valid_id(guild_id) || !valid_id(user_id) || !out) {
        return ECONOMY_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    const std::string key = key_for(guild_id, user_id);
    const bool is_new = g_players.find(key) == g_players.end();
    const bool global_is_new =
        g_global_players.find(user_id) == g_global_players.end();
    Player& player = ensure_player(guild_id, user_id);
    const GlobalPlayer global_before = global_is_new
        ? GlobalPlayer{} : g_global_players.at(user_id);
    touch_global_player(user_id, static_cast<int64_t>(std::time(nullptr)));
    if ((is_new || global_is_new) && !save_locked()) {
        if (is_new) g_players.erase(key);
        if (global_is_new) g_global_players.erase(user_id);
        else g_global_players[user_id] = global_before;
        return ECONOMY_STORAGE_ERROR;
    }
    snapshot(player, out);
    return ECONOMY_OK;
}

int claim_impl(const char* guild_id, const char* user_id, int64_t now,
               int64_t* award, int64_t* remaining, bool daily) {
    if (!valid_id(guild_id) || !valid_id(user_id) || now <= 0 || !award || !remaining) {
        return ECONOMY_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    const std::string key = key_for(guild_id, user_id);
    const bool is_new = g_players.find(key) == g_players.end();
    Player& player = ensure_player(guild_id, user_id);
    const Player before = player;
    GuildDynamics& dynamics = g_dynamics[guild_id];
    const GuildDynamics dynamics_before = dynamics;
    const bool chaos_is_new = g_chaos_players.find(key) == g_chaos_players.end();
    ChaosPlayer& chaos = g_chaos_players[key];
    const ChaosPlayer chaos_before = chaos;
    const bool advanced_is_new = g_advanced_players.find(key) == g_advanced_players.end();
    AdvancedPlayer& advanced = g_advanced_players[key];
    const AdvancedPlayer advanced_before = advanced;
    const bool lifecycle_is_new = g_lifecycle.find(key) == g_lifecycle.end();
    FinancialLifecycle& lifecycle = g_lifecycle[key];
    const FinancialLifecycle lifecycle_before = lifecycle;
    const bool development_is_new = g_development.find(key) == g_development.end();
    PlayerDevelopment& development = g_development[key];
    const PlayerDevelopment development_before = development;
    const bool global_is_new =
        g_global_players.find(user_id) == g_global_players.end();
    const GlobalPlayer global_before = global_is_new
        ? GlobalPlayer{} : g_global_players.at(user_id);
    GuildEconomy& guild = g_guilds[guild_id];
    g_display_symbol = g_settings[guild_id].currency_symbol;
    int64_t& last_claim = daily ? player.last_daily_unix : player.last_work_unix;
    const int64_t cooldown = daily ? kDailyCooldown : kWorkCooldown;
    const int64_t elapsed = std::max<int64_t>(0, now - last_claim);
    if (last_claim > 0 && elapsed < cooldown) {
        *award = 0;
        *remaining = cooldown - elapsed;
        if (chaos_is_new) g_chaos_players.erase(key);
        if (advanced_is_new) g_advanced_players.erase(key);
        if (lifecycle_is_new) g_lifecycle.erase(key);
        if (development_is_new) g_development.erase(key);
        return ECONOMY_COOLDOWN;
    }

    const int64_t wage_floor = 1500 + static_cast<int64_t>(chaos.job_tier) * 2500;
    const int64_t wage_ceiling = 4000 + static_cast<int64_t>(chaos.job_tier) * 5500;
    std::uniform_int_distribution<int64_t> distribution(
        daily ? 7500 : wage_floor, daily ? 12500 : wage_ceiling);
    *award = distribution(g_rng);
    *remaining = 0;
    player.wallet_cents += *award;
    last_claim = now;
    if (daily) {
        chaos.savings_cents += chaos.savings_cents / 10000;
        static constexpr int32_t hysa_basis_points[] = {450, 350, 525, 400, 475, 300};
        chaos.hysa_cents += chaos.hysa_cents *
            hysa_basis_points[std::min<uint32_t>(lifecycle.bank_id, 5)] / 3650000;
        chaos.bonds_cents += chaos.bonds_cents / 5000;
        chaos.debt_cents += chaos.debt_cents / 1000;
        advanced.retirement_cents += advanced.retirement_cents / 3000;
        advanced.index_fund_cents += advanced.index_fund_cents / 4000;
        const int commodity_move = std::uniform_int_distribution<int>(-8, 12)(g_rng);
        advanced.commodities_cents = std::max<int64_t>(0,
            advanced.commodities_cents +
            advanced.commodities_cents * commodity_move / 1000);
        advanced.margin_debt_cents += advanced.margin_debt_cents / 500;
        if (!has_managed_property(guild_id, user_id)) {
            const int64_t property_income[] = {0, 1000, 2500, 7500};
            player.wallet_cents += property_income[
                std::min<uint32_t>(chaos.property_level, 3)];
        }
        player.wallet_cents +=
            static_cast<int64_t>(chaos.shares[2]) * guild.prices[2] / 1000 +
            static_cast<int64_t>(chaos.shares[4]) * guild.prices[4] / 800 +
            static_cast<int64_t>(chaos.shares[5]) * guild.prices[5] / 1200;
        if (chaos.debt_cents > 500000) {
            chaos.credit_score = std::max(300, chaos.credit_score - 1);
        }
    } else {
        ++player.work_count;
        ++chaos.experience;
        if (chaos.job_tier == 0) chaos.job_tier = 1;
        if (!lifecycle.last_review_unix || now - lifecycle.last_review_unix >= 24 * 3600) {
            const int review = std::uniform_int_distribution<int>(-8, 12)(g_rng) +
                               (guild.confidence - 50) / 8;
            lifecycle.performance = std::clamp(lifecycle.performance + review, 0, 100);
            lifecycle.last_review_unix = now;
            const bool laid_off = chaos.job_tier > 0 &&
                (lifecycle.performance < 20 ||
                 std::uniform_int_distribution<int>(1, 10000)(g_rng) <
                    std::max(0, guild.unemployment_bp - 500) / 3);
            if (laid_off) {
                chaos.job_tier = 0;
                lifecycle.performance = 50;
                ++lifecycle.layoffs;
            } else {
                const uint32_t xp_required[] = {0, 0, 10, 30, 80};
                const uint32_t next = std::min<uint32_t>(4, chaos.job_tier + 1);
                const bool degree_ok = next < 3 || chaos.degree != 0;
                const bool executive_degree = next < 4 || chaos.degree == 1 || chaos.degree == 6;
                if (next > chaos.job_tier && lifecycle.performance >= 75 &&
                    chaos.experience >= xp_required[next] && degree_ok && executive_degree) {
                    chaos.job_tier = next;
                    lifecycle.performance = 60;
                }
            }
        }
        if (chaos.job_tier >= 2) {
            advanced.retirement_cents += *award * (chaos.job_tier * 2) / 100;
        }
        if (chaos.job_tier) chaos.credit_score = std::min(850, chaos.credit_score + 1);
    }
    if (!daily && chaos_before.job_tier == 0 && chaos.job_tier > 0) {
        ++dynamics.hires;
    } else if (!daily && chaos_before.job_tier > 0 && chaos.job_tier == 0) {
        ++dynamics.layoffs;
    }
    GlobalPlayer& global =
        sync_global_player(user_id, chaos, development, now);
    ++global.lifetime_actions;
    global.achievements |= 1ull << 0;
    if (save_locked()) return ECONOMY_OK;
    if (is_new) {
        g_players.erase(key);
    } else {
        g_players[key] = before;
    }
    if (chaos_is_new) {
        g_chaos_players.erase(key);
    } else {
        g_chaos_players[key] = chaos_before;
    }
    if (advanced_is_new) {
        g_advanced_players.erase(key);
    } else {
        g_advanced_players[key] = advanced_before;
    }
    if (lifecycle_is_new) {
        g_lifecycle.erase(key);
    } else {
        g_lifecycle[key] = lifecycle_before;
    }
    if (development_is_new) g_development.erase(key);
    else g_development[key] = development_before;
    if (global_is_new) g_global_players.erase(user_id);
    else g_global_players[user_id] = global_before;
    g_dynamics[guild_id] = dynamics_before;
    return ECONOMY_STORAGE_ERROR;
}

int economy_claim_daily_impl(const char* guild_id, const char* user_id, int64_t now,
                             int64_t* award, int64_t* remaining) {
    return claim_impl(guild_id, user_id, now, award, remaining, true);
}

int economy_work_impl(const char* guild_id, const char* user_id, int64_t now,
                      int64_t* award, int64_t* remaining) {
    return claim_impl(guild_id, user_id, now, award, remaining, false);
}

int economy_move_money_impl(const char* guild_id, const char* user_id,
                            int64_t amount, int deposit) {
    if (!valid_id(guild_id) || !valid_id(user_id) || amount <= 0 ||
        amount > kMaximumTransaction || (deposit != 0 && deposit != 1)) {
        return ECONOMY_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    const std::string key = key_for(guild_id, user_id);
    const bool is_new = g_players.find(key) == g_players.end();
    Player& player = ensure_player(guild_id, user_id);
    const Player before = player;
    const bool global_is_new =
        g_global_players.find(user_id) == g_global_players.end();
    const GlobalPlayer global_before = global_is_new
        ? GlobalPlayer{} : g_global_players.at(user_id);
    GuildDynamics& dynamics = g_dynamics[guild_id];
    const GuildDynamics dynamics_before = dynamics;
    int64_t& source = deposit ? player.wallet_cents : player.checking_cents;
    int64_t& destination = deposit ? player.checking_cents : player.wallet_cents;
    if (source < amount) {
        if (is_new) g_players.erase(key);
        return ECONOMY_INSUFFICIENT_FUNDS;
    }
    source -= amount;
    destination += amount;
    if (deposit) add_bounded(dynamics.saving_cents, amount / 4);
    GlobalPlayer& global = touch_global_player(
        user_id, static_cast<int64_t>(std::time(nullptr)));
    ++global.lifetime_actions;
    global.achievements |= 1ull << 0;
    if (save_locked()) return ECONOMY_OK;
    if (is_new) {
        g_players.erase(key);
    } else {
        g_players[key] = before;
    }
    if (global_is_new) g_global_players.erase(user_id);
    else g_global_players[user_id] = global_before;
    g_dynamics[guild_id] = dynamics_before;
    return ECONOMY_STORAGE_ERROR;
}

int economy_transfer_impl(const char* guild_id, const char* from_user,
                          const char* to_user, int64_t amount) {
    if (!valid_id(guild_id) || !valid_id(from_user) || !valid_id(to_user) ||
        amount <= 0 || amount > kMaximumTransaction) {
        return ECONOMY_INVALID_ARGUMENT;
    }
    if (std::string(from_user) == to_user) return ECONOMY_SELF_TRANSFER;

    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    const std::string sender_key = key_for(guild_id, from_user);
    const std::string recipient_key = key_for(guild_id, to_user);
    const bool sender_is_new = g_players.find(sender_key) == g_players.end();
    Player& initial_sender = ensure_player(guild_id, from_user);
    if (amount > 10000000 && initial_sender.work_count < 10) {
        if (sender_is_new) g_players.erase(sender_key);
        return ECONOMY_INVALID_ARGUMENT;
    }
    if (initial_sender.wallet_cents < amount) {
        if (sender_is_new) g_players.erase(sender_key);
        return ECONOMY_INSUFFICIENT_FUNDS;
    }
    const bool recipient_is_new = g_players.find(recipient_key) == g_players.end();
    ensure_player(guild_id, to_user);
    // The recipient insertion may rehash the map, so reacquire both references.
    Player& sender = g_players.at(sender_key);
    Player& recipient = g_players.at(recipient_key);
    if (recipient.wallet_cents > kMaximumTransaction - amount) {
        if (sender_is_new) g_players.erase(sender_key);
        if (recipient_is_new) g_players.erase(recipient_key);
        return ECONOMY_INVALID_ARGUMENT;
    }
    const Player sender_before = sender;
    const Player recipient_before = recipient;
    GuildDynamics& dynamics = g_dynamics[guild_id];
    const GuildDynamics dynamics_before = dynamics;
    const bool global_was_new =
        g_global_players.find(from_user) == g_global_players.end();
    const GlobalPlayer global_before = global_was_new
        ? GlobalPlayer{} : g_global_players.at(from_user);
    const bool recipient_global_was_new =
        g_global_players.find(to_user) == g_global_players.end();
    const GlobalPlayer recipient_global_before = recipient_global_was_new
        ? GlobalPlayer{} : g_global_players.at(to_user);
    sender.wallet_cents -= amount;
    recipient.wallet_cents += amount;
    add_bounded(dynamics.spending_cents, amount);
    GlobalPlayer& global = touch_global_player(
        from_user, static_cast<int64_t>(std::time(nullptr)));
    global.lifetime_trade_cents += static_cast<uint64_t>(amount);
    ++global.lifetime_actions;
    global.achievements |= 1ull << 0;
    touch_global_player(to_user, static_cast<int64_t>(std::time(nullptr)));
    if (save_locked()) return ECONOMY_OK;
    if (sender_is_new) {
        g_players.erase(sender_key);
    } else {
        g_players[sender_key] = sender_before;
    }
    if (recipient_is_new) {
        g_players.erase(recipient_key);
    } else {
        g_players[recipient_key] = recipient_before;
    }
    g_dynamics[guild_id] = dynamics_before;
    if (global_was_new) g_global_players.erase(from_user);
    else g_global_players[from_user] = global_before;
    if (recipient_global_was_new) g_global_players.erase(to_user);
    else g_global_players[to_user] = recipient_global_before;
    return ECONOMY_STORAGE_ERROR;
}

int economy_buy_item_impl(const char* guild_id, const char* user_id,
                          uint32_t item_index, uint32_t quantity, int64_t unit_price) {
    if (!valid_id(guild_id) || !valid_id(user_id) ||
        item_index >= ECONOMY_ITEM_COUNT || quantity == 0 || quantity > 100 ||
        unit_price <= 0 || unit_price > kMaximumTransaction / quantity) {
        return ECONOMY_INVALID_ARGUMENT;
    }

    const int64_t total = unit_price * quantity;
    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    const std::string key = key_for(guild_id, user_id);
    const bool is_new = g_players.find(key) == g_players.end();
    Player& player = ensure_player(guild_id, user_id);
    if (player.wallet_cents < total) {
        if (is_new) g_players.erase(key);
        return ECONOMY_INSUFFICIENT_FUNDS;
    }
    if (player.items[item_index] > std::numeric_limits<uint32_t>::max() - quantity) {
        return ECONOMY_INVALID_ARGUMENT;
    }
    const Player before = player;
    const bool global_is_new =
        g_global_players.find(user_id) == g_global_players.end();
    const GlobalPlayer global_before = global_is_new
        ? GlobalPlayer{} : g_global_players.at(user_id);
    GuildDynamics& dynamics = g_dynamics[guild_id];
    const GuildDynamics dynamics_before = dynamics;
    player.wallet_cents -= total;
    player.items[item_index] += quantity;
    add_bounded(dynamics.spending_cents, total);
    GlobalPlayer& global = touch_global_player(
        user_id, static_cast<int64_t>(std::time(nullptr)));
    ++global.lifetime_actions;
    global.lifetime_trade_cents += static_cast<uint64_t>(total);
    global.achievements |= 1ull << 0;
    if (save_locked()) return ECONOMY_OK;
    if (is_new) {
        g_players.erase(key);
    } else {
        g_players[key] = before;
    }
    if (global_is_new) g_global_players.erase(user_id);
    else g_global_players[user_id] = global_before;
    g_dynamics[guild_id] = dynamics_before;
    return ECONOMY_STORAGE_ERROR;
}

int64_t advanced_asset_value(const AdvancedPlayer& player, const GuildEconomy& guild);
int64_t property_equity(const std::string& guild_id, const std::string& user_id);
int64_t business_tangible_value(const std::string& guild_id,
                                const std::string& user_id);

size_t economy_leaderboard_impl(const char* guild_id, EconomyLeaderboardEntry* out,
                                size_t capacity) {
    if (!valid_id(guild_id) || !out || capacity == 0) return 0;

    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    std::vector<std::pair<std::string, int64_t>> entries;
    const std::string prefix = std::string(guild_id) + '\x1f';
    const auto guild_it = g_guilds.find(guild_id);
    for (const auto& entry : g_players) {
        if (entry.first.compare(0, prefix.size(), prefix) == 0) {
            int64_t worth = entry.second.wallet_cents + entry.second.checking_cents;
            const auto chaos_it = g_chaos_players.find(entry.first);
            if (chaos_it != g_chaos_players.end()) {
                const ChaosPlayer& player = chaos_it->second;
                worth += player.savings_cents + player.hysa_cents + player.bonds_cents +
                         player.business_cash_cents - player.debt_cents;
                worth += property_equity(guild_id, entry.first.substr(prefix.size()));
                worth += business_tangible_value(
                    guild_id, entry.first.substr(prefix.size()));
                if (guild_it != g_guilds.end()) {
                    for (size_t i = 0; i < kStockCount; ++i) {
                        worth += static_cast<int64_t>(player.shares[i]) *
                                 guild_it->second.prices[i];
                    }
                }
            }
            const auto advanced_it = g_advanced_players.find(entry.first);
            if (advanced_it != g_advanced_players.end() && guild_it != g_guilds.end()) {
                worth += advanced_asset_value(advanced_it->second, guild_it->second);
            }
            entries.emplace_back(entry.first.substr(prefix.size()), worth);
        }
    }
    std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) return left.second > right.second;
        return left.first < right.first;
    });

    const size_t count = std::min({capacity, entries.size(),
                                   static_cast<size_t>(ECONOMY_MAX_LEADERBOARD)});
    for (size_t i = 0; i < count; ++i) {
        std::fill(std::begin(out[i].user_id), std::end(out[i].user_id), '\0');
        entries[i].first.copy(out[i].user_id, sizeof(out[i].user_id) - 1);
        out[i].net_worth_cents = entries[i].second;
    }
    return count;
}

int economy_flush_impl() {
    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    return save_locked() ? ECONOMY_OK : ECONOMY_STORAGE_ERROR;
}

constexpr std::array<const char*, kStockCount> kTickers = {
    "MEOW", "RAT", "YUM", "INT", "ENRN", "LPE", "SKIB", "BOOG", "DOGM"
};
constexpr std::array<const char*, kStockCount> kCompanyNames = {
    "MEOW", "Rat Mining", "Yummy Burger", "Intelligent", "Enron",
    "Lobster Power & Electric", "Skibidi Steel", "Boogle",
    "Dogecoin Mines"
};
constexpr std::array<const char*, kStockCount> kCompanyPersonalities = {
    "consumer electronics; announcement-sensitive",
    "mining and raw materials; discovery-driven",
    "food service; consumer-spending sensitive",
    "enterprise technology; stable with breakthrough risk",
    "extremely volatile; every filing causes concern",
    "regulated energy; rate and infrastructure sensitive",
    "manufacturing; construction-cycle sensitive",
    "internet technology; growth-expectation sensitive",
    "meme extraction; speculation is the business model"
};
constexpr std::array<const char*, 4> kEconomyPersonalities = {
    "Stable Economy", "High-Growth Economy",
    "Financial Hub", "Chaotic Economy"
};
constexpr std::array<const char*, 4> kNewsRarity = {
    "COMMON", "UNCOMMON", "RARE", "LEGENDARY"
};
constexpr std::array<const char*, 5> kJobs = {
    "Unemployed", "Fast-food Survivor", "Skilled Technician",
    "Financial Analyst", "Unaccountably Powerful Executive"
};
constexpr std::array<const char*, 6> kBanks = {
    "Bank of Miyamii", "Horen", "Sisi Princeton",
    "Kitty Express", "Discover", "Capital Two"
};
constexpr std::array<int32_t, 6> kBankAprBasisPoints = {
    900, 1250, 750, 650, 800, 1500
};
constexpr std::array<int32_t, 6> kBankMinimumCredit = {
    520, 480, 600, 700, 500, 300
};
constexpr std::array<int32_t, 6> kBankLoanLimitPercent = {
    100, 140, 90, 160, 75, 55
};
constexpr std::array<int32_t, 6> kBankHysaBasisPoints = {
    450, 350, 525, 400, 475, 300
};
constexpr std::array<const char*, 10> kDegrees = {
    "None", "Finance", "Economics", "Accounting", "Computer Science",
    "Engineering", "Business", "Marketing", "Law", "Psychology"
};
constexpr std::array<const char*, 4> kProperties = {
    "None", "Apartment", "House", "Commercial Tower"
};
constexpr std::array<int64_t, 4> kPropertyBaseValues = {
    0, 750000, 2500000, 10000000
};
constexpr std::array<int64_t, 4> kPropertyDailyRent = {
    0, 1500, 4500, 18000
};
constexpr std::array<const char*, 6> kIndustries = {
    "Unspecified", "Food & Hospitality", "Manufacturing",
    "Technology", "Logistics", "Retail"
};
constexpr std::array<const char*, 6> kIndustrySlugs = {
    "", "food", "manufacturing", "tech", "logistics", "retail"
};
constexpr std::array<int64_t, 6> kIndustryMaterialCost = {
    0, 1800, 4200, 6500, 3000, 2200
};
constexpr std::array<int64_t, 6> kIndustrySalePrice = {
    0, 4800, 11000, 18000, 7600, 6000
};

int32_t highest_education(uint32_t mask) {
    for (int32_t degree = 9; degree >= 1; --degree) {
        if (mask & (1u << degree)) return degree;
    }
    return 0;
}

uint32_t achievement_count(uint64_t mask) {
    uint32_t count = 0;
    while (mask) {
        count += static_cast<uint32_t>(mask & 1u);
        mask >>= 1;
    }
    return count;
}

size_t player_server_count(const std::string& user_id) {
    size_t count = 0;
    const std::string suffix = std::string(1, '\x1f') + user_id;
    for (const auto& entry : g_players) {
        if (entry.first.size() >= suffix.size() &&
            entry.first.compare(entry.first.size() - suffix.size(),
                                suffix.size(), suffix) == 0) {
            ++count;
        }
    }
    return count;
}

size_t global_collectible_count(const std::string& user_id) {
    return static_cast<size_t>(std::count_if(
        g_collectible_assets.begin(), g_collectible_assets.end(),
        [&](const CollectibleAsset& collectible) {
            return collectible.owner_id == user_id;
        }));
}

GlobalPlayer& sync_global_player(const std::string& user_id,
                                 ChaosPlayer& local,
                                 PlayerDevelopment& development,
                                 int64_t now) {
    GlobalPlayer& global = touch_global_player(user_id, now);
    if (local.degree > 0 && local.degree <= 9) {
        global.education_mask |= 1u << local.degree;
    }
    global.licenses |= development.certifications;
    local.degree = std::max<uint32_t>(
        local.degree, static_cast<uint32_t>(
            highest_education(global.education_mask)));
    development.certifications |= global.licenses;
    return global;
}

void add_bounded(int64_t& target, int64_t amount) {
    if (amount <= 0) return;
    target = std::min<int64_t>(
        kMaximumTransaction,
        target > kMaximumTransaction - amount
            ? kMaximumTransaction : target + amount);
}

std::string event_headline(size_t company, bool positive, uint32_t rarity,
                           uint32_t stage) {
    const std::string name = kCompanyNames[company % kStockCount];
    if (stage == 0) {
        if (company == 0) {
            return positive
                ? "Rumor: MEOW is preparing a major consumer-tech announcement."
                : "MEOW suppliers warn that its secret project may be slipping.";
        }
        if (company == 1) {
            return positive
                ? "Rat Mining survey crews report unusually shiny core samples."
                : "Rat Mining warns that a major shaft needs emergency repairs.";
        }
        if (company == 2) {
            return positive
                ? "Yummy Burger reports packed dining rooms and suspiciously happy franchisees."
                : "Food inflation squeezes Yummy Burger margins.";
        }
        if (company == 3) {
            return positive
                ? "Intelligent researchers hint at an enterprise-computing breakthrough."
                : "Intelligent delays a closely watched platform rollout.";
        }
        if (company == 4) {
            return positive
                ? "Enron files an earnings report containing several positive numbers."
                : "Enron executives schedule an emergency call, which feels traditional.";
        }
        return positive
            ? name + " reports stronger demand and expanding production."
            : name + " warns of weaker orders and rising costs.";
    }
    if (rarity >= 3 && positive) {
        return name + " delivers a generation-defining breakthrough; world markets reprice.";
    }
    return positive
        ? name + " confirms the rumors with a stronger-than-expected announcement."
        : name + " disappoints speculators; the crowded trade unwinds violently.";
}

void apply_news_impact(const std::string& guild_id, const NewsEvent& event,
                       GuildEconomy& economy, GuildExchange& exchange) {
    if (event.company >= 0 && event.company < int(kStockCount)) {
        const size_t company = static_cast<size_t>(event.company);
        exchange.sentiment[company] = std::clamp(
            exchange.sentiment[company] + event.impact, 0, 100);
        economy.prices[company] = std::clamp<int64_t>(
            economy.prices[company] +
                economy.prices[company] * event.impact / 250,
            50, 1000000000);
    }
    economy.confidence = std::clamp(
        economy.confidence + event.impact / 5, 10, 90);
    (void)guild_id;
}

void create_news_event(const std::string& guild_id, int64_t now,
                       size_t company, bool positive, uint32_t rarity,
                       GuildEconomy& economy, GuildExchange& exchange) {
    NewsEvent event;
    event.id = g_next_news_id++;
    event.origin_guild = guild_id;
    event.created_unix = now;
    event.evolves_unix = now + (rarity >= 2 ? 6 : 12) * 3600;
    event.company = static_cast<int32_t>(company % kStockCount);
    event.rarity = std::min<uint32_t>(3, rarity);
    event.positive = positive;
    event.impact = (positive ? 1 : -1) *
        static_cast<int32_t>(4 + event.rarity * 3);
    event.headline = event_headline(company, positive, event.rarity, 0);
    apply_news_impact(guild_id, event, economy, exchange);
    g_news.push_back(std::move(event));
    if (g_news.size() > 200) {
        g_news.erase(g_news.begin(), g_news.begin() + (g_news.size() - 200));
    }
}

void update_economic_dynamics(const std::string& guild_id,
                              GuildEconomy& economy,
                              GuildExchange& exchange,
                              int64_t step_time) {
    GuildDynamics& dynamics = g_dynamics[guild_id];
    GovernmentState& government = g_governments[guild_id];
    if (!dynamics.last_cycle_unix) dynamics.last_cycle_unix = step_time - 3600;

    const int64_t demand = dynamics.spending_cents / 100000 +
        dynamics.investment_cents / 150000 +
        static_cast<int64_t>(dynamics.hires) * 2 -
        dynamics.saving_cents / 180000 -
        dynamics.selling_cents / 150000 +
        (dynamics.capital_inflow_cents - dynamics.capital_outflow_cents) / 200000 +
        (dynamics.exports_cents - dynamics.imports_cents) / 250000 -
        static_cast<int64_t>(dynamics.layoffs) * 3;
    dynamics.trend = static_cast<int32_t>(
        std::clamp<int64_t>(demand, -25, 25));

    economy.confidence = std::clamp(
        economy.confidence + dynamics.trend / 3, 10, 90);
    economy.inflation_bp = std::clamp(
        economy.inflation_bp +
            static_cast<int32_t>(std::clamp<int64_t>(
                dynamics.spending_cents / 300000 +
                dynamics.capital_outflow_cents / 500000 -
                dynamics.saving_cents / 450000 -
                (dynamics.policy_rate_bp - 400) / 80,
                -80, 120)),
        -100, 1800);
    economy.unemployment_bp = std::clamp(
        economy.unemployment_bp -
            static_cast<int32_t>(std::min<uint32_t>(100, dynamics.hires * 12)) +
            static_cast<int32_t>(std::min<uint32_t>(150, dynamics.layoffs * 18)) -
            dynamics.trend * 2,
        150, 3000);

    if (economy.inflation_bp > 650) {
        dynamics.policy_rate_bp =
            std::min(2000, dynamics.policy_rate_bp + 25);
    } else if (economy.confidence < 38 || economy.unemployment_bp > 1000) {
        dynamics.policy_rate_bp =
            std::max(50, dynamics.policy_rate_bp - 25);
    } else if (dynamics.policy_rate_bp > 400) {
        dynamics.policy_rate_bp -= 10;
    } else if (dynamics.policy_rate_bp < 400) {
        dynamics.policy_rate_bp += 10;
    }

    const int64_t external_balance =
        dynamics.capital_inflow_cents + dynamics.exports_cents -
        dynamics.capital_outflow_cents - dynamics.imports_cents;
    dynamics.currency_index = std::clamp(
        dynamics.currency_index +
            (economy.confidence - 50) * 2 -
            economy.inflation_bp / 80 +
            static_cast<int32_t>(
                std::clamp<int64_t>(external_balance / 250000, -100, 100)),
        1000, 50000);

    const bool recession = economy.confidence < 35 ||
                           economy.unemployment_bp > 1200 ||
                           dynamics.trend <= -10;
    if (recession) {
        ++dynamics.recession_hours;
        dynamics.recovery_hours = 0;
    } else if (dynamics.recession_hours) {
        ++dynamics.recovery_hours;
        if (dynamics.recovery_hours >= 6) {
            dynamics.recession_hours = 0;
            dynamics.recovery_hours = 0;
        }
    }

    if (dynamics.recession_hours >= 12 &&
        step_time - dynamics.last_stimulus_unix >= 24 * 3600) {
        const int64_t stimulus = 50000;
        if (government.treasury_cents < stimulus) {
            const int64_t issuance = stimulus - government.treasury_cents;
            government.treasury_cents += issuance;
            dynamics.government_debt_cents += issuance;
        }
        government.treasury_cents -= stimulus;
        economy.confidence = std::min(90, economy.confidence + 4);
        economy.unemployment_bp = std::max(150, economy.unemployment_bp - 100);
        dynamics.spending_cents += stimulus;
        dynamics.last_stimulus_unix = step_time;
    }

    if (dynamics.forex_volume_cents > 500000 ||
        dynamics.currency_index > 12500) {
        dynamics.personality = 2;
    } else if (economy.confidence > 68 &&
               economy.unemployment_bp < 600) {
        dynamics.personality = 1;
    } else if (std::abs(dynamics.trend) >= 12 ||
               economy.inflation_bp > 900) {
        dynamics.personality = 3;
    } else {
        dynamics.personality = 0;
    }

    for (const NewsEvent& event : g_news) {
        if (!event.global || event.id <= dynamics.last_global_event_id) continue;
        apply_news_impact(guild_id, event, economy, exchange);
        dynamics.last_global_event_id =
            std::max(dynamics.last_global_event_id, event.id);
    }
    for (NewsEvent& event : g_news) {
        if (event.origin_guild != guild_id || event.stage != 0 ||
            event.evolves_unix > step_time) continue;
        event.stage = 1;
        const size_t company = static_cast<size_t>(
            std::max<int32_t>(0, event.company));
        const bool fulfilled = exchange.profit[company] >= 0
            ? event.positive : !event.positive;
        event.positive = fulfilled;
        event.impact = (fulfilled ? 1 : -1) *
            static_cast<int32_t>(7 + event.rarity * 4);
        event.headline =
            event_headline(company, fulfilled, event.rarity, 1);
        if (event.rarity >= 2 && fulfilled) {
            event.global = true;
            dynamics.last_global_event_id =
                std::max(dynamics.last_global_event_id, event.id);
        }
        apply_news_impact(guild_id, event, economy, exchange);
    }

    const uint64_t hour = static_cast<uint64_t>(step_time / 3600);
    if ((hour + std::hash<std::string>{}(guild_id)) % 6 == 0) {
        const size_t company = static_cast<size_t>(
            (hour + std::hash<std::string>{}(guild_id)) % kStockCount);
        const bool positive = dynamics.trend > 0 ||
            (dynamics.trend == 0 && hour % 2 == 0);
        const uint32_t rarity = static_cast<uint32_t>(
            std::abs(dynamics.trend) >= 20 ? 3 :
            std::abs(dynamics.trend) >= 12 ? 2 :
            std::abs(dynamics.trend) >= 6 ? 1 : 0);
        create_news_event(guild_id, step_time, company, positive, rarity,
                          economy, exchange);
        dynamics.expectations[company] =
            positive ? 8 + int(rarity) * 4 : -8 - int(rarity) * 4;
        dynamics.rumor_due[company] =
            step_time + (rarity >= 2 ? 6 : 12) * 3600;
    }

    dynamics.spending_cents /= 4;
    dynamics.investment_cents /= 4;
    dynamics.saving_cents /= 4;
    dynamics.selling_cents /= 4;
    dynamics.capital_inflow_cents /= 4;
    dynamics.capital_outflow_cents /= 4;
    dynamics.exports_cents /= 4;
    dynamics.imports_cents /= 4;
    dynamics.forex_volume_cents /= 2;
    dynamics.hires /= 2;
    dynamics.layoffs /= 2;
    dynamics.last_cycle_unix = step_time;
}

std::string cash(int64_t cents) {
    const bool negative = cents < 0;
    const uint64_t value = negative
        ? static_cast<uint64_t>(-(cents + 1)) + 1
        : static_cast<uint64_t>(cents);
    std::ostringstream out;
    if (negative) out << '-';
    out << g_display_symbol << value / 100 << '.' << std::setw(2)
        << std::setfill('0') << value % 100;
    return out.str();
}

std::string currency_amount(int64_t cents, const std::string& symbol) {
    const std::string previous = g_display_symbol;
    g_display_symbol = symbol;
    const std::string result = cash(cents);
    g_display_symbol = previous;
    return result;
}

bool parse_cash(std::string value, int64_t& cents) {
    value.erase(std::remove(value.begin(), value.end(), '$'), value.end());
    if (!g_display_symbol.empty() && value.rfind(g_display_symbol, 0) == 0) {
        value.erase(0, g_display_symbol.size());
    }
    value.erase(std::remove(value.begin(), value.end(), ','), value.end());
    if (value.empty() || value[0] == '-') return false;
    const size_t dot = value.find('.');
    std::string whole = dot == std::string::npos ? value : value.substr(0, dot);
    std::string fraction = dot == std::string::npos ? "" : value.substr(dot + 1);
    if (whole.empty()) whole = "0";
    if (fraction.size() > 2 || (dot != std::string::npos &&
        value.find('.', dot + 1) != std::string::npos)) return false;
    while (fraction.size() < 2) fraction += '0';
    const auto digits = [](const std::string& text) {
        return !text.empty() && std::all_of(text.begin(), text.end(),
            [](unsigned char ch) { return ch >= '0' && ch <= '9'; });
    };
    if (!digits(whole) || !digits(fraction)) return false;
    try {
        const uint64_t dollars = std::stoull(whole);
        const uint64_t total = dollars * 100 + std::stoul(fraction);
        if (!total || total > static_cast<uint64_t>(kMaximumTransaction)) return false;
        cents = static_cast<int64_t>(total);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> words(const char* args) {
    std::istringstream input(args ? args : "");
    std::vector<std::string> result;
    std::string word;
    while (input >> word) result.push_back(word);
    return result;
}

void set_output(char* output, size_t capacity, const std::string& message) {
    if (!output || capacity == 0) return;
    const size_t count = std::min(capacity - 1, message.size());
    std::memcpy(output, message.data(), count);
    output[count] = '\0';
}

size_t stock_index(std::string ticker) {
    std::transform(ticker.begin(), ticker.end(), ticker.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    for (size_t i = 0; i < kTickers.size(); ++i) {
        if (ticker == kTickers[i]) return i;
    }
    return kStockCount;
}

std::string normalize_user(std::string value) {
    if (value.size() >= 4 && value.front() == '<' && value.back() == '>') {
        if (value.compare(0, 3, "<@!") == 0) {
            value = value.substr(3, value.size() - 4);
        } else if (value.compare(0, 2, "<@") == 0) {
            value = value.substr(2, value.size() - 3);
        }
    }
    return valid_id(value.c_str()) ? value : std::string();
}

uint32_t industry_index(std::string slug) {
    std::transform(slug.begin(), slug.end(), slug.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    for (uint32_t i = 1; i < kIndustrySlugs.size(); ++i) {
        if (slug == kIndustrySlugs[i]) return i;
    }
    return 0;
}

int64_t business_tangible_value(const std::string& guild_id,
                                const std::string& user_id) {
    const auto found = g_business_profiles.find(key_for(guild_id, user_id));
    if (found == g_business_profiles.end()) return 0;
    const BusinessProfile& business = found->second;
    const uint32_t industry = std::min<uint32_t>(business.industry, 5);
    return static_cast<int64_t>(business.equipment_level) * 150000 +
           static_cast<int64_t>(business.raw_materials) *
               kIndustryMaterialCost[industry] +
           static_cast<int64_t>(business.finished_goods) *
               kIndustrySalePrice[industry] -
           business.debt_cents;
}

uint32_t active_partner_share(const std::string& guild_id,
                              const std::string& owner_id,
                              std::string* partner_id = nullptr) {
    for (const BusinessPartnership& partnership : g_partnerships) {
        if (partnership.guild_id == guild_id &&
            partnership.owner_id == owner_id && partnership.accepted) {
            if (partner_id) *partner_id = partnership.partner_id;
            return partnership.share_basis_points;
        }
    }
    return 0;
}

int64_t property_market_value(const GuildEconomy& guild, uint32_t tier,
                              uint32_t condition = 100) {
    tier = std::min<uint32_t>(tier, 3);
    const int64_t macro_index = std::clamp<int64_t>(
        8500 + guild.confidence * 25 + guild.inflation_bp / 4 -
        guild.unemployment_bp / 8, 6500, 14000);
    const int64_t condition_index = 7000 + std::min<uint32_t>(condition, 100) * 30;
    return kPropertyBaseValues[tier] * macro_index / 10000 *
           condition_index / 10000;
}

int64_t property_equity(const std::string& guild_id, const std::string& user_id) {
    int64_t total = 0;
    bool managed = false;
    for (const PropertyAsset& property : g_properties) {
        if (property.guild_id != guild_id || property.owner_id != user_id) continue;
        managed = true;
        total += std::max<int64_t>(0, property.market_value_cents -
                                     property.mortgage_cents);
    }
    if (managed) return total;
    const auto chaos = g_chaos_players.find(key_for(guild_id, user_id));
    if (chaos == g_chaos_players.end()) return 0;
    return kPropertyBaseValues[
        std::min<uint32_t>(chaos->second.property_level, 3)];
}

void recalculate_property_level(const std::string& guild_id,
                                const std::string& user_id) {
    uint32_t level = 0;
    for (const PropertyAsset& property : g_properties) {
        if (property.guild_id == guild_id && property.owner_id == user_id) {
            level = std::max(level, property.tier);
        }
    }
    g_chaos_players[key_for(guild_id, user_id)].property_level = level;
}

PropertyAsset* find_property(const std::string& guild_id, uint64_t id) {
    auto found = std::find_if(g_properties.begin(), g_properties.end(),
        [&](const PropertyAsset& property) {
            return property.guild_id == guild_id && property.id == id;
        });
    return found == g_properties.end() ? nullptr : &*found;
}

void materialize_legacy_property(const std::string& guild_id,
                                 const std::string& user_id,
                                 ChaosPlayer& player, int64_t now) {
    if (!player.property_level || has_managed_property(guild_id, user_id)) return;
    const uint32_t tier = std::min<uint32_t>(player.property_level, 3);
    const int64_t value = property_market_value(g_guilds[guild_id], tier);
    g_properties.push_back({g_next_property_id++, guild_id, user_id, tier,
                            kPropertyBaseValues[tier], value, 0, 0, now, 100, 0, false});
}

uint32_t collectible_rarity(uint64_t serial) {
    return serial % 97 == 0 ? 3 : serial % 10 == 0 ? 2 :
           serial % 3 == 0 ? 1 : 0;
}

int64_t collectible_market_price(const std::string& guild_id,
                                 const GuildEconomy& guild) {
    uint64_t supply = 0;
    uint64_t transfers = 0;
    for (const CollectibleAsset& collectible : g_collectible_assets) {
        if (collectible.guild_id != guild_id) continue;
        ++supply;
        transfers += collectible.transfers;
    }
    const int64_t demand_index = std::clamp<int64_t>(
        70 + guild.confidence +
        std::min<uint64_t>(40, transfers * 2) -
        std::min<uint64_t>(30, supply / 20),
        60, 200);
    return 10000 * demand_index / 120;
}

void mint_collectible(const std::string& guild_id, const std::string& owner_id,
                      int64_t now) {
    const uint64_t serial = g_next_collectible_serial++;
    g_collectible_assets.push_back({
        serial, guild_id, owner_id, "", now, now, 0, 0,
        collectible_rarity(serial)
    });
}

void materialize_collectibles(const std::string& guild_id,
                              const std::string& owner_id,
                              uint32_t desired, int64_t now) {
    const size_t serialized = static_cast<size_t>(std::count_if(
        g_collectible_assets.begin(), g_collectible_assets.end(),
        [&](const CollectibleAsset& collectible) {
            return collectible.guild_id == guild_id &&
                   collectible.owner_id == owner_id &&
                   collectible.auction_id == 0;
        }));
    for (size_t count = serialized; count < desired; ++count) {
        mint_collectible(guild_id, owner_id, now);
    }
}

void settle_auction(const AuctionListing& auction, int64_t now) {
    if (auction.highest_bidder_id.empty()) {
        if (auction.asset_type == 0) {
            g_advanced_players[key_for(auction.guild_id, auction.seller_id)].collectibles +=
                auction.quantity;
            for (CollectibleAsset& collectible : g_collectible_assets) {
                if (collectible.guild_id == auction.guild_id &&
                    collectible.auction_id == auction.id) {
                    collectible.auction_id = 0;
                }
            }
        } else if (PropertyAsset* property =
                       find_property(auction.guild_id, auction.asset_id)) {
            property->listed = false;
        }
        g_audit.push_back({auction.guild_id, auction.seller_id, now, "auction",
                           "Listing #" + std::to_string(auction.id) +
                           " expired without a buyer."});
        return;
    }

    const int64_t fee = std::max<int64_t>(1, auction.highest_bid_cents / 50);
    int64_t mortgage_payoff = 0;
    PropertyAsset* sold_property = auction.asset_type == 1
        ? find_property(auction.guild_id, auction.asset_id) : nullptr;
    if (sold_property) {
        mortgage_payoff = std::min(
            sold_property->mortgage_cents,
            std::max<int64_t>(0, auction.highest_bid_cents - fee));
        sold_property->mortgage_cents -= mortgage_payoff;
        sold_property->next_payment_unix = 0;
        sold_property->missed_payments = 0;
    }
    ensure_player(auction.guild_id, auction.seller_id).wallet_cents +=
        auction.highest_bid_cents - fee - mortgage_payoff;
    g_governments[auction.guild_id].treasury_cents += fee;
    g_crime_players[
        key_for(auction.guild_id, auction.seller_id)].taxes_paid_cents += fee;
    if (auction.asset_type == 0) {
        g_advanced_players[
            key_for(auction.guild_id, auction.highest_bidder_id)].collectibles +=
            auction.quantity;
        for (CollectibleAsset& collectible : g_collectible_assets) {
            if (collectible.guild_id != auction.guild_id ||
                collectible.auction_id != auction.id) continue;
            collectible.previous_owner_id = collectible.owner_id;
            collectible.owner_id = auction.highest_bidder_id;
            collectible.acquired_unix = now;
            collectible.auction_id = 0;
            ++collectible.transfers;
        }
    } else if (PropertyAsset* property = sold_property) {
        const std::string old_owner = property->owner_id;
        property->owner_id = auction.highest_bidder_id;
        property->listed = false;
        recalculate_property_level(auction.guild_id, old_owner);
        recalculate_property_level(auction.guild_id, auction.highest_bidder_id);
    }
    g_audit.push_back({auction.guild_id, auction.seller_id, now, "auction",
        "Listing #" + std::to_string(auction.id) + " sold for " +
        cash(auction.highest_bid_cents) + " after a " + cash(fee) + " market fee."});
    g_audit.push_back({auction.guild_id, auction.highest_bidder_id, now, "auction",
        "Won listing #" + std::to_string(auction.id) + " for " +
        cash(auction.highest_bid_cents) + "."});
}

constexpr std::array<int, kStockCount> kStockVolatility = {
    7, 10, 4, 8, 18, 7, 10, 8, 28
};

int64_t stock_spread(const GuildExchange& exchange, size_t stock,
                     int64_t midpoint) {
    const int64_t liquidity_penalty =
        80000 / std::max<uint32_t>(20, exchange.liquidity[stock]);
    const int64_t basis_points = std::clamp<int64_t>(
        20 + kStockVolatility[stock] * 3 + liquidity_penalty, 25, 900);
    return std::max<int64_t>(2, midpoint * basis_points / 10000);
}

int64_t stock_fundamental_value(const GuildExchange& exchange, size_t stock) {
    const int64_t equity = std::max<int64_t>(
        0, exchange.cash[stock] - exchange.debt[stock]);
    const int64_t earnings_value = exchange.profit[stock] > 0
        ? exchange.profit[stock] * 720 : 0;
    return std::max<int64_t>(
        50, (equity + earnings_value) /
            static_cast<int64_t>(
                std::max<uint64_t>(1, exchange.shares_outstanding[stock])));
}

void cancel_stock_orders(const std::string& guild_id, size_t stock) {
    for (auto order = g_orders.begin(); order != g_orders.end();) {
        if (order->guild_id != guild_id || order->stock != stock) {
            ++order;
            continue;
        }
        const std::string key = key_for(guild_id, order->user_id);
        if (order->buy) {
            ensure_player(guild_id, order->user_id).wallet_cents +=
                order->limit_price_cents * order->quantity;
        } else {
            g_chaos_players[key].shares[stock] += order->quantity;
        }
        order = g_orders.erase(order);
    }
}

void apply_stock_split(const std::string& guild_id, size_t stock,
                       GuildEconomy& guild, GuildExchange& exchange) {
    guild.prices[stock] = std::max<int64_t>(50, guild.prices[stock] / 2);
    exchange.shares_outstanding[stock] =
        std::min<uint64_t>(std::numeric_limits<uint64_t>::max() / 2,
                           exchange.shares_outstanding[stock]) * 2;
    const std::string prefix = guild_id + '\x1f';
    for (auto& entry : g_chaos_players) {
        if (entry.first.compare(0, prefix.size(), prefix) != 0) continue;
        entry.second.shares[stock] =
            entry.second.shares[stock] > std::numeric_limits<uint32_t>::max() / 2
            ? std::numeric_limits<uint32_t>::max()
            : entry.second.shares[stock] * 2;
    }
    for (auto& entry : g_advanced_players) {
        if (entry.first.compare(0, prefix.size(), prefix) != 0) continue;
        entry.second.short_shares[stock] =
            entry.second.short_shares[stock] > std::numeric_limits<uint32_t>::max() / 2
            ? std::numeric_limits<uint32_t>::max()
            : entry.second.short_shares[stock] * 2;
    }
    for (LimitOrder& order : g_orders) {
        if (order.guild_id != guild_id || order.stock != stock) continue;
        order.quantity = order.quantity > std::numeric_limits<uint32_t>::max() / 2
            ? std::numeric_limits<uint32_t>::max() : order.quantity * 2;
        order.limit_price_cents = std::max<int64_t>(1, order.limit_price_cents / 2);
    }
}

void update_market(const std::string& guild_id, GuildEconomy& guild, int64_t now) {
    GuildExchange& exchange = g_exchanges[guild_id];
    if (guild.last_market_unix <= 0) {
        guild.last_market_unix = now;
        for (size_t i = 0; i < kStockCount; ++i) {
            exchange.profit[i] = exchange.revenue[i] - exchange.expenses[i];
        }
        return;
    }
    int64_t steps = (now - guild.last_market_unix) / 3600;
    steps = std::clamp<int64_t>(steps, 0, 72);
    if (!steps) return;
    std::uniform_int_distribution<int> noise(-100, 100);
    for (int64_t step = 0; step < steps; ++step) {
        const int64_t step_time = guild.last_market_unix + (step + 1) * 3600;
        for (size_t i = 0; i < guild.prices.size(); ++i) {
            exchange.remaining_liquidity[i] = exchange.liquidity[i];
            const uint64_t npc_volume = static_cast<uint64_t>(
                std::uniform_int_distribution<uint32_t>(
                    exchange.liquidity[i] / 3,
                    std::max<uint32_t>(1, exchange.liquidity[i]))(g_rng));
            exchange.volume[i] = exchange.volume[i] * 23 / 24 + npc_volume;

            if (!exchange.listed[i]) {
                if (step_time >= exchange.halted_until[i]) {
                    exchange.listed[i] = true;
                    exchange.halted_until[i] = 0;
                    exchange.sentiment[i] = 30;
                    exchange.distress[i] = 0;
                    exchange.debt[i] /= 2;
                    guild.prices[i] = 500;
                }
                continue;
            }
            if (exchange.halted_until[i] > step_time) {
                continue;
            }

            const int sector_bias =
                i == 2 ? (50 - guild.confidence) / 4 :
                i == 1 || i == 6 ? (guild.confidence - 50) / 3 -
                                    guild.inflation_bp / 300 :
                i == 8 ? (guild.confidence - 50) :
                (guild.confidence - 50) / 2;
            const int revenue_move = std::clamp(
                sector_bias + noise(g_rng) / 8, -25, 25);
            exchange.revenue[i] = std::max<int64_t>(
                10000, exchange.revenue[i] +
                exchange.revenue[i] * revenue_move / 1000);
            exchange.expenses[i] = std::max<int64_t>(
                10000, exchange.expenses[i] +
                exchange.expenses[i] *
                    std::clamp(guild.inflation_bp / 100 + noise(g_rng) / 15,
                               -15, 30) / 1000);
            exchange.profit[i] = exchange.revenue[i] - exchange.expenses[i];
            exchange.cash[i] = std::max<int64_t>(
                0, exchange.cash[i] + exchange.profit[i]);
            if (exchange.profit[i] < 0) {
                exchange.debt[i] += -exchange.profit[i] / 4 +
                                    exchange.debt[i] / 100000;
            } else {
                exchange.debt[i] = std::max<int64_t>(
                    0, exchange.debt[i] - exchange.profit[i] / 20);
            }
            const int64_t fundamental = stock_fundamental_value(exchange, i);
            exchange.sentiment[i] = std::clamp(
                exchange.sentiment[i] + sector_bias / 5 + noise(g_rng) / 20,
                0, 100);
            int pressure = static_cast<int>(std::clamp<int64_t>(
                (fundamental - guild.prices[i]) * 80 /
                    std::max<int64_t>(50, guild.prices[i]), -45, 45));
            pressure += (exchange.sentiment[i] - 50) / 3;
            pressure += sector_bias / 2;
            pressure += g_dynamics[guild_id].expectations[i];
            pressure += noise(g_rng) * kStockVolatility[i] / 100;
            if (i == 4 && noise(g_rng) > 96) pressure -= 240;
            if (i == 8 && noise(g_rng) > 75) pressure += noise(g_rng) / 2;
            pressure = std::clamp(pressure, -300, 180);
            const int64_t old_price = guild.prices[i];
            guild.prices[i] = std::clamp<int64_t>(
                old_price + old_price * pressure / 1000, 50, 1000000000);
            if (std::llabs(guild.prices[i] - old_price) * 100 >= old_price * 20) {
                exchange.halted_until[i] = step_time + 3600;
            }
            if (exchange.profit[i] < 0 && exchange.debt[i] > exchange.cash[i]) {
                ++exchange.distress[i];
            } else if (exchange.distress[i]) {
                --exchange.distress[i];
            }
            if (exchange.distress[i] >= 24) {
                exchange.listed[i] = false;
                exchange.halted_until[i] = step_time + 24 * 3600;
                exchange.distress[i] = 0;
                guild.prices[i] = 50;
                cancel_stock_orders(guild_id, i);
                const std::string prefix = guild_id + '\x1f';
                for (auto& player : g_chaos_players) {
                    if (player.first.compare(0, prefix.size(), prefix) == 0) {
                        player.second.shares[i] = 0;
                    }
                }
                for (auto& player : g_advanced_players) {
                    if (player.first.compare(0, prefix.size(), prefix) == 0) {
                        player.second.short_shares[i] = 0;
                    }
                }
                continue;
            }
            if ((step_time / 3600) % 24 == 0 &&
                exchange.debt[i] > exchange.cash[i]) {
                const uint64_t old_shares = exchange.shares_outstanding[i];
                exchange.shares_outstanding[i] +=
                    std::max<uint64_t>(1, old_shares / 20);
                guild.prices[i] = guild.prices[i] * old_shares /
                    exchange.shares_outstanding[i];
                exchange.sentiment[i] = std::max(0, exchange.sentiment[i] - 3);
            }
            if (guild.prices[i] >= 200000) {
                apply_stock_split(guild_id, i, guild, exchange);
            }
        }
        update_economic_dynamics(guild_id, guild, exchange, step_time);
        for (int32_t& expectation : g_dynamics[guild_id].expectations) {
            expectation = expectation * 3 / 4;
        }
        guild.confidence = std::clamp(
            guild.confidence + noise(g_rng) / 80, 10, 90);
        guild.event_id = g_news.empty()
            ? guild.event_id : static_cast<uint32_t>(g_news.back().id);
    }
    guild.last_market_unix += steps * 3600;
}

bool process_orders(const std::string& guild_id, GuildEconomy& guild,
                    GuildExchange& exchange, int64_t now) {
    bool changed = false;
    for (auto it = g_orders.begin(); it != g_orders.end();) {
        if (it->guild_id != guild_id) {
            ++it;
            continue;
        }
        if (!exchange.listed[it->stock] ||
            exchange.halted_until[it->stock] > now ||
            !exchange.remaining_liquidity[it->stock]) {
            ++it;
            continue;
        }
        const int64_t midpoint = guild.prices[it->stock];
        const int64_t spread = stock_spread(exchange, it->stock, midpoint);
        const int64_t execution = it->buy
            ? midpoint + (spread + 1) / 2
            : std::max<int64_t>(1, midpoint - spread / 2);
        const bool fills = (it->buy && execution <= it->limit_price_cents) ||
                           (!it->buy && execution >= it->limit_price_cents);
        if (!fills) {
            ++it;
            continue;
        }
        uint32_t fill = std::min(it->quantity,
            exchange.remaining_liquidity[it->stock]);
        const std::string key = key_for(it->guild_id, it->user_id);
        if (it->buy) {
            const uint32_t room = std::numeric_limits<uint32_t>::max() -
                                  g_chaos_players[key].shares[it->stock];
            fill = std::min(fill, room);
            if (!fill) { ++it; continue; }
            g_chaos_players[key].shares[it->stock] += fill;
            const int64_t refund = (it->limit_price_cents - execution) * fill;
            g_players[key].wallet_cents += std::max<int64_t>(0, refund);
        } else {
            g_players[key].wallet_cents += execution * fill;
        }
        it->quantity -= fill;
        exchange.remaining_liquidity[it->stock] -= fill;
        exchange.volume[it->stock] += fill;
        const int direction = it->buy ? 1 : -1;
        guild.prices[it->stock] = std::clamp<int64_t>(
            guild.prices[it->stock] +
            direction * std::max<int64_t>(
                1, guild.prices[it->stock] * fill /
                    std::max<uint32_t>(1000, exchange.liquidity[it->stock] * 20)),
            50, 1000000000);
        changed = true;
        if (!it->quantity) it = g_orders.erase(it);
        else ++it;
    }
    return changed;
}

bool update_bank_network(const std::string& guild_id,
                         const GuildEconomy& economy, int64_t now) {
    GuildBankNetwork& network = g_bank_networks[guild_id];
    if (!network.last_update_unix) {
        network.last_update_unix = now;
        return true;
    }
    int64_t steps = std::clamp<int64_t>(
        (now - network.last_update_unix) / 3600, 0, 72);
    if (!steps) return false;
    std::array<uint32_t, 6> defaults{};
    const std::string prefix = guild_id + '\x1f';
    for (const auto& entry : g_lifecycle) {
        if (entry.first.compare(0, prefix.size(), prefix) != 0) continue;
        defaults[std::min<uint32_t>(entry.second.bank_id, 5)] +=
            entry.second.defaults;
    }
    std::uniform_int_distribution<int> noise(-2, 2);
    for (int64_t step = 0; step < steps; ++step) {
        const int64_t step_time = network.last_update_unix + (step + 1) * 3600;
        for (size_t bank = 0; bank < network.stability.size(); ++bank) {
            if (network.failed_until[bank] > 0) {
                if (step_time >= network.failed_until[bank]) {
                    network.failed_until[bank] = 0;
                    network.stability[bank] = 40;
                }
                continue;
            }
            network.stability[bank] = std::clamp(
                network.stability[bank] + noise(g_rng) +
                (economy.confidence - 50) / 20 -
                std::max(0, economy.unemployment_bp - 500) / 1000 -
                static_cast<int>(std::min<uint32_t>(5, defaults[bank] / 3)),
                0, 100);
            if (network.stability[bank] > 10) continue;
            network.failed_until[bank] = step_time + 24 * 3600;
            network.stability[bank] = 10;
            for (auto& lifecycle : g_lifecycle) {
                if (lifecycle.first.compare(0, prefix.size(), prefix) != 0 ||
                    lifecycle.second.bank_id != bank) continue;
                ChaosPlayer& player = g_chaos_players[lifecycle.first];
                const int64_t insured_limit = 1000000;
                const int64_t savings_loss =
                    std::max<int64_t>(0, player.savings_cents - insured_limit) / 10;
                const int64_t hysa_loss =
                    std::max<int64_t>(0, player.hysa_cents - insured_limit) / 5;
                player.savings_cents -= savings_loss;
                player.hysa_cents -= hysa_loss;
                const size_t split = lifecycle.first.find('\x1f');
                if (split != std::string::npos) {
                    g_audit.push_back({guild_id, lifecycle.first.substr(split + 1),
                        step_time, "bank-resolution",
                        "Bank entered resolution; insured deposits protected, uninsured loss: " +
                        cash(savings_loss + hysa_loss)});
                }
            }
        }
    }
    network.last_update_unix += steps * 3600;
    return true;
}

int64_t portfolio_value(const ChaosPlayer& player, const GuildEconomy& guild) {
    int64_t total = player.bonds_cents + player.business_cash_cents;
    for (size_t i = 0; i < kStockCount; ++i) {
        total += static_cast<int64_t>(player.shares[i]) * guild.prices[i];
    }
    return total;
}

int64_t advanced_asset_value(const AdvancedPlayer& player, const GuildEconomy& guild) {
    int64_t total = player.cd_cents + player.retirement_cents +
                    player.index_fund_cents + player.commodities_cents +
                    player.casino_reserve_cents;
    for (size_t i = 0; i < kStockCount; ++i) {
        total -= static_cast<int64_t>(player.short_shares[i]) * guild.prices[i];
    }
    return total - player.margin_debt_cents;
}

EconomySnapshot calculate_snapshot(const std::string& guild_id, int64_t now) {
    EconomySnapshot snapshot;
    snapshot.guild_id = guild_id;
    snapshot.timestamp = now;
    const GuildEconomy& guild = g_guilds[guild_id];
    snapshot.stock_prices = guild.prices;
    const std::string prefix = guild_id + '\x1f';
    std::vector<int64_t> net_worths;
    uint64_t defaults = 0;
    for (const auto& entry : g_players) {
        if (entry.first.compare(0, prefix.size(), prefix) != 0) continue;
        ++snapshot.active_players;
        const Player& wallet = entry.second;
        int64_t net_worth = wallet.wallet_cents + wallet.checking_cents;
        snapshot.money_supply_cents += wallet.wallet_cents + wallet.checking_cents;
        for (uint32_t quantity : wallet.items) snapshot.item_supply += quantity;

        auto chaos_it = g_chaos_players.find(entry.first);
        if (chaos_it != g_chaos_players.end()) {
            const ChaosPlayer& player = chaos_it->second;
            const int64_t invested = portfolio_value(player, guild);
            net_worth += player.savings_cents + player.hysa_cents +
                         invested +
                         property_equity(guild_id, entry.first.substr(prefix.size())) -
                         player.debt_cents;
            net_worth += business_tangible_value(
                guild_id, entry.first.substr(prefix.size()));
            snapshot.money_supply_cents += player.savings_cents + player.hysa_cents +
                                           player.business_cash_cents;
            snapshot.total_debt_cents += player.debt_cents;
            for (const PropertyAsset& property : g_properties) {
                if (property.guild_id == guild_id &&
                    property.owner_id == entry.first.substr(prefix.size())) {
                    snapshot.total_debt_cents += property.mortgage_cents;
                }
            }
            snapshot.business_value_cents += player.business_cash_cents;
            snapshot.business_value_cents += business_tangible_value(
                guild_id, entry.first.substr(prefix.size()));
            const auto business_it = g_business_profiles.find(entry.first);
            if (business_it != g_business_profiles.end()) {
                snapshot.total_debt_cents += business_it->second.debt_cents;
                snapshot.item_supply += business_it->second.raw_materials +
                                        business_it->second.finished_goods;
            }
            snapshot.investment_value_cents += invested - player.business_cash_cents +
                property_equity(guild_id, entry.first.substr(prefix.size()));
            if (player.job_tier) ++snapshot.employed_players;
        }
        auto advanced_it = g_advanced_players.find(entry.first);
        if (advanced_it != g_advanced_players.end()) {
            const AdvancedPlayer& player = advanced_it->second;
            const int64_t advanced_value = advanced_asset_value(player, guild);
            net_worth += advanced_value;
            snapshot.money_supply_cents += player.cd_cents + player.retirement_cents +
                player.index_fund_cents + player.commodities_cents +
                player.casino_reserve_cents;
            snapshot.total_debt_cents += player.margin_debt_cents;
            snapshot.business_value_cents += player.casino_reserve_cents;
            snapshot.investment_value_cents += advanced_value;
            snapshot.item_supply += player.collectibles;
        }
        auto lifecycle_it = g_lifecycle.find(entry.first);
        if (lifecycle_it != g_lifecycle.end()) defaults += lifecycle_it->second.defaults;
        net_worths.push_back(net_worth);
    }
    if (!net_worths.empty()) {
        const int64_t total = std::accumulate(
            net_worths.begin(), net_worths.end(), int64_t{0});
        snapshot.average_net_worth_cents =
            total / static_cast<int64_t>(net_worths.size());
        std::sort(net_worths.begin(), net_worths.end());
        const size_t middle = net_worths.size() / 2;
        snapshot.median_net_worth_cents = net_worths.size() % 2
            ? net_worths[middle]
            : (net_worths[middle - 1] + net_worths[middle]) / 2;
    }
    const int debt_pressure = snapshot.money_supply_cents > 0
        ? static_cast<int>(std::min<long double>(
            60, static_cast<long double>(snapshot.total_debt_cents) * 50 /
                snapshot.money_supply_cents))
        : snapshot.total_debt_cents > 0 ? 60 : 0;
    snapshot.bank_stability = std::clamp(
        100 - debt_pressure - static_cast<int>(std::min<uint64_t>(30, defaults * 3)),
        0, 100);
    const auto bank_network = g_bank_networks.find(guild_id);
    if (bank_network != g_bank_networks.end()) {
        const int total = std::accumulate(
            bank_network->second.stability.begin(),
            bank_network->second.stability.end(), 0);
        snapshot.bank_stability =
            std::min(snapshot.bank_stability, total / 6);
    }
    return snapshot;
}

bool record_snapshot(const std::string& guild_id, int64_t now) {
    int64_t latest = 0;
    for (auto it = g_snapshots.rbegin(); it != g_snapshots.rend(); ++it) {
        if (it->guild_id == guild_id) {
            latest = it->timestamp;
            break;
        }
    }
    if (latest > 0 && now - latest < 3600) return false;
    g_snapshots.push_back(calculate_snapshot(guild_id, now));
    size_t count = static_cast<size_t>(std::count_if(
        g_snapshots.begin(), g_snapshots.end(),
        [&](const EconomySnapshot& snapshot) {
            return snapshot.guild_id == guild_id;
        }));
    while (count > 168) {
        auto oldest = std::find_if(g_snapshots.begin(), g_snapshots.end(),
            [&](const EconomySnapshot& snapshot) {
                return snapshot.guild_id == guild_id;
            });
        if (oldest == g_snapshots.end()) break;
        g_snapshots.erase(oldest);
        --count;
    }
    const std::string prefix = guild_id + '\x1f';
    const GuildEconomy& guild = g_guilds[guild_id];
    for (const auto& entry : g_players) {
        if (entry.first.compare(0, prefix.size(), prefix) != 0) continue;
        const std::string user_id = entry.first.substr(prefix.size());
        const Player& wallet = entry.second;
        const ChaosPlayer chaos = g_chaos_players.count(entry.first)
            ? g_chaos_players.at(entry.first) : ChaosPlayer{};
        const AdvancedPlayer advanced = g_advanced_players.count(entry.first)
            ? g_advanced_players.at(entry.first) : AdvancedPlayer{};
        const int64_t liquid = wallet.wallet_cents + wallet.checking_cents +
                               chaos.savings_cents + chaos.hysa_cents;
        const int64_t invested = portfolio_value(chaos, guild) +
            advanced_asset_value(advanced, guild) +
            property_equity(guild_id, user_id) +
            business_tangible_value(guild_id, user_id);
        int64_t visible_debt = chaos.debt_cents + advanced.margin_debt_cents;
        const auto business = g_business_profiles.find(entry.first);
        if (business != g_business_profiles.end()) {
            visible_debt += business->second.debt_cents;
        }
        for (const PropertyAsset& property : g_properties) {
            if (property.guild_id == guild_id && property.owner_id == user_id) {
                visible_debt += property.mortgage_cents;
            }
        }
        g_player_history.push_back({
            guild_id, user_id, now, liquid + invested - chaos.debt_cents,
            liquid, visible_debt, invested
        });
        size_t player_points = static_cast<size_t>(std::count_if(
            g_player_history.begin(), g_player_history.end(),
            [&](const PlayerHistoryPoint& point) {
                return point.guild_id == guild_id && point.user_id == user_id;
            }));
        while (player_points > 168) {
            auto oldest = std::find_if(
                g_player_history.begin(), g_player_history.end(),
                [&](const PlayerHistoryPoint& point) {
                    return point.guild_id == guild_id && point.user_id == user_id;
                });
            if (oldest == g_player_history.end()) break;
            g_player_history.erase(oldest);
            --player_points;
        }
    }
    if (g_player_history.size() > 100000) {
        g_player_history.erase(
            g_player_history.begin(),
            g_player_history.begin() + (g_player_history.size() - 100000));
    }
    return true;
}

int economy_tick_all_impl(int64_t now) {
    if (now <= 0) return ECONOMY_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    bool changed = false;
    for (auto& entry : g_guilds) {
        GuildEconomy& guild = entry.second;
        if (guild.last_market_unix <= 0 || now - guild.last_market_unix >= 3600) {
            update_market(entry.first, guild, now);
            changed = true;
        }
        changed = process_orders(
            entry.first, guild, g_exchanges[entry.first], now) || changed;
        changed = update_bank_network(entry.first, guild, now) || changed;
    }
    for (auto& entry : g_governments) {
        GovernmentState& government = entry.second;
        if (government.election_end_unix <= 0 || now < government.election_end_unix) {
            continue;
        }
        std::string winner;
        uint32_t winning_platform = 0;
        size_t winning_votes = 0;
        for (const ElectionCandidate& candidate : g_candidates) {
            if (candidate.guild_id != entry.first) continue;
            const std::string prefix = entry.first + '\x1f';
            const size_t votes = static_cast<size_t>(std::count_if(
                g_votes.begin(), g_votes.end(), [&](const auto& vote) {
                    return vote.first.compare(0, prefix.size(), prefix) == 0 &&
                           vote.second == candidate.user_id;
                }));
            if (winner.empty() || votes > winning_votes ||
                (votes == winning_votes && candidate.user_id < winner)) {
                winner = candidate.user_id;
                winning_platform = candidate.platform;
                winning_votes = votes;
            }
        }
        if (!winner.empty()) {
            government.mayor_id = winner;
            government.platform = winning_platform;
            government.term_end_unix = now + 7 * 24 * 3600;
            if (winning_platform == 1) {
                government.tax_basis_points = 800;
                government.welfare_cents = 5000;
            } else if (winning_platform == 2) {
                government.tax_basis_points = 400;
                government.welfare_cents = 2000;
            } else {
                government.tax_basis_points = 250;
                government.welfare_cents = 1000;
            }
            GuildEconomy& economy = g_guilds[entry.first];
            economy.confidence = std::min(90, economy.confidence +
                (winning_platform == 2 ? 5 : winning_platform == 0 ? 3 : 1));
            if (winning_platform == 1 || winning_platform == 2) {
                economy.unemployment_bp = std::max(
                    150, economy.unemployment_bp - (winning_platform == 2 ? 200 : 100));
            }
            g_audit.push_back({entry.first, winner, now, "election",
                "Won the mayoral election with " + std::to_string(winning_votes) + " vote(s)."});
        }
        government.election_end_unix = 0;
        g_candidates.erase(std::remove_if(g_candidates.begin(), g_candidates.end(),
            [&](const ElectionCandidate& candidate) {
                return candidate.guild_id == entry.first;
            }), g_candidates.end());
        const std::string vote_prefix = entry.first + '\x1f';
        for (auto vote = g_votes.begin(); vote != g_votes.end();) {
            if (vote->first.compare(0, vote_prefix.size(), vote_prefix) == 0) {
                vote = g_votes.erase(vote);
            } else {
                ++vote;
            }
        }
        changed = true;
    }
    for (auto& entry : g_crime_players) {
        CrimePlayer& crime = entry.second;
        if (crime.jailed_until_unix > 0 && now >= crime.jailed_until_unix) {
            crime.jailed_until_unix = 0;
            changed = true;
        }
        if (!crime.heat) continue;
        if (!crime.last_heat_decay_unix) {
            crime.last_heat_decay_unix =
                crime.last_crime_unix > 0 ? crime.last_crime_unix : now;
        }
        const int64_t days = (now - crime.last_heat_decay_unix) / (24 * 3600);
        if (days > 0) {
            const uint32_t reduction = static_cast<uint32_t>(
                std::min<int64_t>(crime.heat, days * 5));
            crime.heat -= reduction;
            crime.last_heat_decay_unix += days * 24 * 3600;
            changed = true;
        }
    }
    for (auto auction = g_auctions.begin(); auction != g_auctions.end();) {
        if (auction->end_unix > now) {
            ++auction;
            continue;
        }
        settle_auction(*auction, now);
        auction = g_auctions.erase(auction);
        changed = true;
    }
    for (auto property = g_properties.begin(); property != g_properties.end();) {
        GuildEconomy& economy = g_guilds[property->guild_id];
        Player& owner_wallet = ensure_player(property->guild_id, property->owner_id);
        const int64_t income_days = property->last_income_unix > 0
            ? std::clamp<int64_t>((now - property->last_income_unix) / (24 * 3600), 0, 30)
            : 0;
        if (income_days > 0) {
            const int64_t daily_rent = kPropertyDailyRent[property->tier] *
                std::max<int64_t>(25, property->condition) *
                std::max<int64_t>(25, economy.confidence) / 5000;
            owner_wallet.wallet_cents += daily_rent * income_days;
            property->condition = static_cast<uint32_t>(std::max<int64_t>(
                25, static_cast<int64_t>(property->condition) - income_days * 2));
            property->last_income_unix += income_days * 24 * 3600;
            g_audit.push_back({property->guild_id, property->owner_id, now, "rent",
                "Property #" + std::to_string(property->id) + " produced " +
                cash(daily_rent * income_days) + " over " +
                std::to_string(income_days) + " day(s)."});
            changed = true;
        }
        property->market_value_cents =
            property_market_value(economy, property->tier, property->condition);

        bool foreclosed = false;
        int payment_periods = property->mortgage_cents > 0 &&
                              property->next_payment_unix > 0 &&
                              now >= property->next_payment_unix
            ? static_cast<int>(std::min<int64_t>(
                7, 1 + (now - property->next_payment_unix) / (24 * 3600)))
            : 0;
        while (payment_periods-- > 0 && property->mortgage_cents > 0) {
            const int64_t payment = std::min(property->mortgage_cents,
                std::max<int64_t>(1000, property->purchase_price_cents / 60) +
                property->mortgage_cents / 1000);
            int64_t paid = std::min(payment, owner_wallet.checking_cents);
            owner_wallet.checking_cents -= paid;
            const int64_t from_wallet =
                std::min(payment - paid, owner_wallet.wallet_cents);
            owner_wallet.wallet_cents -= from_wallet;
            paid += from_wallet;
            property->mortgage_cents -= std::min(property->mortgage_cents, paid);
            if (paid >= payment) {
                property->missed_payments = 0;
                g_chaos_players[
                    key_for(property->guild_id, property->owner_id)].credit_score =
                    std::min(850, g_chaos_players[
                        key_for(property->guild_id, property->owner_id)].credit_score + 2);
            } else {
                ++property->missed_payments;
                property->mortgage_cents +=
                    std::max<int64_t>(500, property->mortgage_cents / 200);
                g_chaos_players[
                    key_for(property->guild_id, property->owner_id)].credit_score =
                    std::max(300, g_chaos_players[
                        key_for(property->guild_id, property->owner_id)].credit_score - 45);
                if (property->missed_payments >= 3) foreclosed = true;
            }
            property->next_payment_unix += 24 * 3600;
            changed = true;
            if (foreclosed) break;
        }
        if (property->mortgage_cents <= 0) {
            property->mortgage_cents = 0;
            property->next_payment_unix = 0;
            property->missed_payments = 0;
        }
        if (!foreclosed) {
            ++property;
            continue;
        }

        const std::string guild_id = property->guild_id;
        const std::string owner_id = property->owner_id;
        const uint64_t property_id = property->id;
        for (auto auction = g_auctions.begin(); auction != g_auctions.end();) {
            if (auction->asset_type == 1 && auction->asset_id == property_id &&
                auction->guild_id == guild_id) {
                if (!auction->highest_bidder_id.empty()) {
                    ensure_player(guild_id, auction->highest_bidder_id).wallet_cents +=
                        auction->highest_bid_cents;
                }
                auction = g_auctions.erase(auction);
            } else {
                ++auction;
            }
        }
        g_audit.push_back({guild_id, owner_id, now, "foreclosure",
            "Property #" + std::to_string(property_id) +
            " was seized after three missed mortgage payments."});
        property = g_properties.erase(property);
        recalculate_property_level(guild_id, owner_id);
        changed = true;
    }
    for (ScheduledAgreement& agreement : g_agreements) {
        if (!agreement.active || !agreement.accepted ||
            agreement.next_payment_unix <= 0 ||
            now < agreement.next_payment_unix) continue;
        int periods = static_cast<int>(std::min<int64_t>({
            static_cast<int64_t>(agreement.remaining_payments), 30,
            1 + (now - agreement.next_payment_unix) / (24 * 3600)}));
        while (periods-- > 0 && agreement.active &&
               agreement.remaining_payments > 0) {
            bool paid = false;
            if (agreement.type == 0) {
                ChaosPlayer& employer =
                    g_chaos_players[key_for(agreement.guild_id, agreement.issuer_id)];
                if (employer.business_level &&
                    employer.business_cash_cents >= agreement.payment_cents) {
                    employer.business_cash_cents -= agreement.payment_cents;
                    ensure_player(agreement.guild_id,
                                  agreement.counterparty_id).wallet_cents +=
                        agreement.payment_cents;
                    paid = true;
                } else {
                    g_business_profiles[
                        key_for(agreement.guild_id, agreement.issuer_id)].reputation =
                        std::max(0, g_business_profiles[
                            key_for(agreement.guild_id, agreement.issuer_id)].reputation - 10);
                }
            } else {
                Player& tenant =
                    ensure_player(agreement.guild_id, agreement.counterparty_id);
                if (tenant.wallet_cents >= agreement.payment_cents) {
                    tenant.wallet_cents -= agreement.payment_cents;
                    ensure_player(agreement.guild_id,
                                  agreement.issuer_id).wallet_cents +=
                        agreement.payment_cents;
                    paid = true;
                } else {
                    g_chaos_players[
                        key_for(agreement.guild_id,
                                agreement.counterparty_id)].credit_score =
                        std::max(300, g_chaos_players[
                            key_for(agreement.guild_id,
                                    agreement.counterparty_id)].credit_score - 15);
                }
            }
            if (!paid) {
                agreement.active = false;
                g_audit.push_back({agreement.guild_id, agreement.issuer_id, now,
                    "agreement-default",
                    "Scheduled agreement #" + std::to_string(agreement.id) +
                    " defaulted for nonpayment."});
                changed = true;
                break;
            }
            --agreement.remaining_payments;
            agreement.next_payment_unix += 24 * 3600;
            changed = true;
            if (!agreement.remaining_payments) {
                agreement.active = false;
                g_audit.push_back({agreement.guild_id, agreement.issuer_id, now,
                    "agreement-complete",
                    "Scheduled agreement #" + std::to_string(agreement.id) +
                    " completed all payments."});
            }
        }
    }
    for (auto& entry : g_business_profiles) {
        BusinessProfile& business = entry.second;
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const std::string guild_id = entry.first.substr(0, split);
        const std::string owner_id = entry.first.substr(split + 1);
        ChaosPlayer& owner = g_chaos_players[entry.first];
        if (!business.last_decay_unix) business.last_decay_unix = now;
        const int64_t decay_days =
            std::clamp<int64_t>((now - business.last_decay_unix) / (24 * 3600), 0, 30);
        if (decay_days > 0) {
            business.marketing = static_cast<uint32_t>(std::max<int64_t>(
                0, static_cast<int64_t>(business.marketing) - decay_days * 2));
            business.last_decay_unix += decay_days * 24 * 3600;
            changed = true;
        }
        int periods = business.debt_cents > 0 && business.next_payment_unix > 0 &&
                      now >= business.next_payment_unix
            ? static_cast<int>(std::min<int64_t>(
                7, 1 + (now - business.next_payment_unix) / (24 * 3600)))
            : 0;
        while (periods-- > 0 && business.debt_cents > 0) {
            business.debt_cents += std::max<int64_t>(1, business.debt_cents / 5000);
            const int64_t payment = std::min(
                business.debt_cents,
                std::max<int64_t>(1000, business.debt_cents / 30));
            const int64_t paid = std::min(payment, owner.business_cash_cents);
            owner.business_cash_cents -= paid;
            business.debt_cents -= paid;
            if (paid >= payment) {
                business.missed_payments = 0;
                owner.credit_score = std::min(850, owner.credit_score + 2);
                g_audit.push_back({guild_id, owner_id, now, "businessloan",
                    "Company autopay collected " + cash(paid) + "."});
            } else {
                ++business.missed_payments;
                business.debt_cents +=
                    std::max<int64_t>(500, business.debt_cents / 100);
                owner.credit_score = std::max(300, owner.credit_score - 30);
                g_audit.push_back({guild_id, owner_id, now, "businessloan",
                    "Company missed payment; delinquency fee added."});
            }
            business.next_payment_unix += 24 * 3600;
            changed = true;
            if (business.missed_payments >= 3) {
                owner.debt_cents += business.debt_cents;
                business.debt_cents = 0;
                business.next_payment_unix = 0;
                business.missed_payments = 0;
                ++business.defaults;
                business.equipment_level = business.marketing =
                    business.raw_materials = business.finished_goods = 0;
                business.reputation = 10;
                owner.business_cash_cents = 0;
                owner.business_level = 0;
                g_partnerships.erase(std::remove_if(
                    g_partnerships.begin(), g_partnerships.end(),
                    [&](const BusinessPartnership& partnership) {
                        return partnership.guild_id == guild_id &&
                               partnership.owner_id == owner_id;
                    }), g_partnerships.end());
                GuildEconomy& economy = g_guilds[guild_id];
                economy.confidence = std::max(10, economy.confidence - 4);
                economy.unemployment_bp =
                    std::min(3000, economy.unemployment_bp + 100);
                g_audit.push_back({guild_id, owner_id, now, "business-default",
                    "Company liquidated after three missed loan payments; debt became personal."});
                break;
            }
        }
        if (business.debt_cents <= 0) {
            business.debt_cents = 0;
            business.next_payment_unix = 0;
            business.missed_payments = 0;
        }
    }
    for (auto& entry : g_lifecycle) {
        FinancialLifecycle& lifecycle = entry.second;
        auto chaos_it = g_chaos_players.find(entry.first);
        auto wallet_it = g_players.find(entry.first);
        if (chaos_it == g_chaos_players.end() || wallet_it == g_players.end() ||
            chaos_it->second.debt_cents <= 0 || lifecycle.next_payment_unix <= 0 ||
            now < lifecycle.next_payment_unix) {
            continue;
        }
        const size_t split = entry.first.find('\x1f');
        if (split == std::string::npos) continue;
        const std::string guild_id = entry.first.substr(0, split);
        const std::string user_id = entry.first.substr(split + 1);
        g_display_symbol = g_settings[guild_id].currency_symbol;
        ChaosPlayer& player = chaos_it->second;
        Player& wallet = wallet_it->second;
        int periods = static_cast<int>(std::min<int64_t>(
            30, 1 + (now - lifecycle.next_payment_unix) / (24 * 3600)));
        while (periods-- > 0 && player.debt_cents > 0) {
            const int64_t minimum = std::min(player.debt_cents,
                std::max<int64_t>(500, player.debt_cents / 20));
            int64_t paid = std::min(minimum, wallet.checking_cents);
            wallet.checking_cents -= paid;
            int64_t remainder = minimum - paid;
            const int64_t from_wallet = std::min(remainder, wallet.wallet_cents);
            wallet.wallet_cents -= from_wallet;
            paid += from_wallet;
            player.debt_cents -= paid;
            if (paid >= minimum) {
                lifecycle.missed_payments = 0;
                player.credit_score = std::min(850, player.credit_score + 2);
                g_audit.push_back({guild_id, user_id, now, "autopay",
                    "Scheduled debt payment collected: " + cash(paid)});
            } else {
                ++lifecycle.missed_payments;
                const int64_t late_fee = std::max<int64_t>(250, player.debt_cents / 100);
                player.debt_cents += late_fee;
                player.credit_score = std::max(300, player.credit_score - 35);
                g_audit.push_back({guild_id, user_id, now, "delinquency",
                    "Missed scheduled payment; late fee: " + cash(late_fee)});
                if (lifecycle.missed_payments >= 3) {
                    const int64_t seized = std::min(player.savings_cents,
                        std::max<int64_t>(500, player.debt_cents / 10));
                    player.savings_cents -= seized;
                    player.debt_cents -= std::min(player.debt_cents, seized);
                    lifecycle.missed_payments = 0;
                    ++lifecycle.defaults;
                    player.credit_score = std::max(300, player.credit_score - 75);
                    g_audit.push_back({guild_id, user_id, now, "default",
                        "Loan default recorded; savings seized: " + cash(seized)});
                }
            }
            lifecycle.next_payment_unix += 24 * 3600;
            changed = true;
        }
        if (player.debt_cents <= 0) {
            player.debt_cents = 0;
            lifecycle.next_payment_unix = 0;
            lifecycle.missed_payments = 0;
        }
    }
    if (g_audit.size() > 2000) {
        g_audit.erase(g_audit.begin(), g_audit.begin() + (g_audit.size() - 2000));
    }
    for (const auto& entry : g_guilds) {
        changed = record_snapshot(entry.first, now) || changed;
    }
    if (!changed) return ECONOMY_OK;
    return save_locked() ? ECONOMY_OK : ECONOMY_STORAGE_ERROR;
}

int economy_get_currency_impl(const char* guild_id, char* symbol, size_t symbol_capacity,
                              char* name, size_t name_capacity) {
    if (!valid_id(guild_id) || !symbol || !symbol_capacity || !name || !name_capacity) {
        return ECONOMY_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    const GuildSettings& settings = g_settings[guild_id];
    set_output(symbol, symbol_capacity, settings.currency_symbol);
    set_output(name, name_capacity, settings.currency_name);
    return ECONOMY_OK;
}

int economy_game_action_impl(const char* guild_id, const char* user_id,
                             const char* action_text, const char* args, int64_t now,
                             char* output, size_t capacity) {
    if (!valid_id(guild_id) || !valid_id(user_id) || !action_text || now <= 0 ||
        !output || capacity == 0) return ECONOMY_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    const std::string key = key_for(guild_id, user_id);
    Player& wallet = ensure_player(guild_id, user_id);
    SecurityProfile& security = g_security[key];
    if (!security.created_unix) security.created_unix = now;
    if (!security.action_window_unix || now < security.action_window_unix ||
        now - security.action_window_unix >= 60) {
        security.action_window_unix = now;
        security.actions_in_window = 0;
    }
    if (security.actions_in_window >= 30) {
        ++security.rejected_actions;
        save_locked();
        set_output(output, capacity,
            "Economy rate limit reached: wait for the current 60-second window.");
        return ECONOMY_COOLDOWN;
    }
    ++security.actions_in_window;
    ChaosPlayer& player = g_chaos_players[key];
    AdvancedPlayer& advanced = g_advanced_players[key];
    CorporatePlayer& corporate = g_corporate_players[key];
    GuildSettings& settings = g_settings[guild_id];
    FinancialLifecycle& lifecycle = g_lifecycle[key];
    CrimePlayer& crime = g_crime_players[key];
    GovernmentState& government = g_governments[guild_id];
    GuildEconomy& guild = g_guilds[guild_id];
    GuildExchange& exchange = g_exchanges[guild_id];
    GuildBankNetwork& bank_network = g_bank_networks[guild_id];
    BusinessProfile& business = g_business_profiles[key];
    PlayerDevelopment& development = g_development[key];
    const bool global_player_was_new =
        g_global_players.find(user_id) == g_global_players.end();
    const GlobalPlayer global_before = global_player_was_new
        ? GlobalPlayer{} : g_global_players.at(user_id);
    GlobalPlayer& global =
        sync_global_player(user_id, player, development, now);
    GuildDynamics& dynamics = g_dynamics[guild_id];
    const Player wallet_before = wallet;
    const ChaosPlayer player_before = player;
    const AdvancedPlayer advanced_before = advanced;
    const CorporatePlayer corporate_before = corporate;
    const GuildSettings settings_before = settings;
    const FinancialLifecycle lifecycle_before = lifecycle;
    const CrimePlayer crime_before = crime;
    const GovernmentState government_before = government;
    const GuildEconomy guild_before = guild;
    const GuildExchange exchange_before = exchange;
    const GuildBankNetwork bank_network_before = bank_network;
    const BusinessProfile business_before = business;
    const PlayerDevelopment development_before = development;
    const SecurityProfile security_before = security;
    const GuildDynamics dynamics_before = dynamics;
    const std::vector<NewsEvent> news_before = g_news;
    const uint64_t next_news_before = g_next_news_id;
    const std::vector<PlayerContract> contracts_before = g_contracts;
    const std::vector<LimitOrder> orders_before = g_orders;
    const std::vector<AuditEntry> audit_before = g_audit;
    const std::vector<ElectionCandidate> candidates_before = g_candidates;
    const std::unordered_map<std::string, std::string> votes_before = g_votes;
    const std::vector<PropertyAsset> properties_before = g_properties;
    const std::vector<AuctionListing> auctions_before = g_auctions;
    const uint64_t next_property_before = g_next_property_id;
    const uint64_t next_auction_before = g_next_auction_id;
    const std::vector<BusinessPartnership> partnerships_before = g_partnerships;
    const uint64_t next_partnership_before = g_next_partnership_id;
    const std::vector<CollectibleAsset> collectibles_before = g_collectible_assets;
    const uint64_t next_collectible_before = g_next_collectible_serial;
    const std::vector<ScheduledAgreement> agreements_before = g_agreements;
    const uint64_t next_agreement_before = g_next_agreement_id;
    std::string cross_target_key;
    Player cross_target_before;
    bool cross_target_changed = false;
    std::string cross_target_guild;
    GuildDynamics cross_target_dynamics_before;
    bool cross_target_dynamics_changed = false;
    std::string cross_advanced_key;
    AdvancedPlayer cross_advanced_before;
    bool cross_advanced_changed = false;
    update_market(guild_id, guild, now);
    process_orders(guild_id, guild, exchange, now);
    materialize_legacy_property(guild_id, user_id, player, now);
    materialize_collectibles(guild_id, user_id, advanced.collectibles, now);
    g_display_symbol = settings.currency_symbol;

    std::string action(action_text);
    std::transform(action.begin(), action.end(), action.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    const std::vector<std::string> input = words(args);
    std::string message;
    int result = ECONOMY_OK;
    if (player.business_level && !business.industry) business.industry = 5;
    if (player.business_level && !business.raw_materials &&
        advanced.business_inventory) {
        business.raw_materials = advanced.business_inventory;
        advanced.business_inventory = 0;
    }

    auto need_cash = [&](int64_t amount) {
        if (amount <= 0 || wallet.wallet_cents < amount) {
            message = "Your wallet cannot finance that decision.";
            result = ECONOMY_INSUFFICIENT_FUNDS;
            return false;
        }
        return true;
    };
    auto parse_amount = [&](size_t index, int64_t& amount) {
        if (input.size() <= index || !parse_cash(input[index], amount)) {
            message = "Provide a positive amount, for example `250.00`.";
            result = ECONOMY_INVALID_ARGUMENT;
            return false;
        }
        return true;
    };

    const bool incarceration_blocks_action =
        crime.jailed_until_unix > now &&
        action != "profile" && action != "history" && action != "crime" &&
        action != "bail" && action != "government" && action != "election" &&
        action != "taxes";

    if (incarceration_blocks_action) {
        message = "You are incarcerated for another **" +
                  std::to_string((crime.jailed_until_unix - now + 59) / 60) +
                  " minute(s)**. Financial empire privileges are temporarily revoked.";
        result = ECONOMY_COOLDOWN;
    } else if (action == "admin") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "currency") {
            if (input.size() < 3 || input[1].empty() || input[1].size() > 6 ||
                input[1].find_first_of("`@#\\") != std::string::npos) {
                message = "Usage: `~econadmin currency <symbol> <name>` (symbol: 1-6 characters).";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                std::string name;
                for (size_t i = 2; i < input.size(); ++i) {
                    if (!name.empty()) name += ' ';
                    name += input[i];
                }
                if (name.size() > 24) {
                    message = "Currency names may contain at most 24 characters.";
                    result = ECONOMY_INVALID_ARGUMENT;
                } else {
                    settings.currency_symbol = input[1];
                    settings.currency_name = name;
                    g_display_symbol = settings.currency_symbol;
                    message = "Server currency is now **" + settings.currency_symbol +
                              " " + settings.currency_name + "**.";
                }
            }
        } else if (operation == "starting" || operation == "stipend") {
            int64_t amount = 0;
            if (!parse_amount(1, amount) || amount > 100000000) {
                message = "Usage: `~econadmin " + operation +
                          " <amount>` (maximum 1,000,000.00).";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                if (operation == "starting") settings.starting_balance_cents = amount;
                else settings.role_stipend_cents = amount;
                message = (operation == "starting"
                    ? "New-player starting balance set to **"
                    : "Per-role daily stipend set to **") + cash(amount) + "**.";
            }
        } else {
            message = "**Server Economy Configuration**\nCurrency: **" +
                settings.currency_symbol + " " + settings.currency_name +
                "**\nNew-player balance: **" + cash(settings.starting_balance_cents) +
                "**\nDaily stipend per role: **" + cash(settings.role_stipend_cents) +
                "**\n`~econadmin currency <symbol> <name>`\n"
                "`~econadmin starting <amount>`\n`~econadmin stipend <amount>`";
        }
    } else if (action == "history") {
        std::ostringstream out;
        out << "**Recent Financial History**\n";
        size_t count = 0;
        for (auto it = g_audit.rbegin(); it != g_audit.rend() && count < 10; ++it) {
            if (it->guild_id == guild_id && it->user_id == user_id) {
                ++count;
                out << '`' << it->action << "` " << it->summary << '\n';
            }
        }
        if (!count) out << "No recorded advanced transactions yet.";
        message = out.str();
    } else if (action == "forex") {
        const std::string operation = input.empty() ? "markets" : input[0];
        if (operation == "markets") {
            std::ostringstream out;
            out << "**Routine Foreign Exchange**\n";
            size_t shown = 0;
            for (const auto& entry : g_guilds) {
                if (shown++ >= 10) break;
                const GuildSettings& market_settings = g_settings[entry.first];
                const GuildDynamics& market = g_dynamics[entry.first];
                out << "`" << entry.first << "` • "
                    << market_settings.currency_symbol << ' '
                    << market_settings.currency_name << " • index **"
                    << market.currency_index << "** • "
                    << kEconomyPersonalities[
                        std::min<uint32_t>(market.personality, 3)] << '\n';
            }
            if (!shown) out << "No connected currency markets yet.\n";
            out << "\n`/forex input: quote <server_id> [amount]`\n"
                   "`/forex input: exchange <server_id> <amount>`";
            message = out.str();
        } else if ((operation == "quote" || operation == "exchange") &&
                   input.size() >= 2 && valid_id(input[1].c_str()) &&
                   input[1] != guild_id) {
            const std::string target_guild = input[1];
            const std::string target_key = key_for(target_guild, user_id);
            auto target_account = g_players.find(target_key);
            int64_t amount = 10000;
            if (input.size() > 2 && !parse_cash(input[2], amount)) {
                message = "Amount must be positive, for example `250.00`.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (target_account == g_players.end()) {
                message = "Open your account in target server `" + target_guild +
                    "` first by using any economy command there.";
                result = ECONOMY_PLAYER_NOT_FOUND;
            } else {
                GuildDynamics& target_dynamics = g_dynamics[target_guild];
                const GuildSettings& target_settings = g_settings[target_guild];
                const int32_t source_index =
                    std::max(1000, dynamics.currency_index);
                const int32_t target_index =
                    std::max(1000, target_dynamics.currency_index);
                const int32_t spread_bp = std::clamp(
                    125 + std::abs(source_index - target_index) / 100,
                    125, 500);
                const int64_t gross = static_cast<int64_t>(
                    static_cast<long double>(amount) * source_index /
                    target_index);
                const int64_t received =
                    std::max<int64_t>(1, gross * (10000 - spread_bp) / 10000);
                const int64_t fee = std::max<int64_t>(0, gross - received);
                if (operation == "quote") {
                    message = "**Foreign-exchange quote**\n" +
                        currency_amount(amount, settings.currency_symbol) +
                        " → **" +
                        currency_amount(received,
                                        target_settings.currency_symbol) +
                        "**\nRate index: **" +
                        std::to_string(source_index) + " / " +
                        std::to_string(target_index) +
                        "** | spread: **" +
                        std::to_string(spread_bp / 100.0) +
                        "%** | embedded fee: **" +
                        currency_amount(fee, target_settings.currency_symbol) +
                        "**\nRates move with confidence, inflation, trade, and capital flow.";
                } else if (target_account->second.wallet_cents >
                           kMaximumTransaction - received) {
                    message = "The target account cannot hold the converted amount.";
                    result = ECONOMY_INVALID_ARGUMENT;
                } else if (!need_cash(amount)) {
                    result = ECONOMY_INSUFFICIENT_FUNDS;
                } else {
                    cross_target_key = target_key;
                    cross_target_before = target_account->second;
                    cross_target_changed = true;
                    cross_target_guild = target_guild;
                    cross_target_dynamics_before = target_dynamics;
                    cross_target_dynamics_changed = true;
                    wallet.wallet_cents -= amount;
                    target_account->second.wallet_cents += received;
                    add_bounded(dynamics.capital_outflow_cents, amount);
                    add_bounded(target_dynamics.capital_inflow_cents, received);
                    add_bounded(dynamics.forex_volume_cents, amount);
                    add_bounded(target_dynamics.forex_volume_cents, received);
                    global.lifetime_forex_cents += static_cast<uint64_t>(amount);
                    global.reputation = std::min(100, global.reputation + 1);
                    message = "Exchanged **" +
                        currency_amount(amount, settings.currency_symbol) +
                        "** into **" +
                        currency_amount(received,
                                        target_settings.currency_symbol) +
                        "** in server `" + target_guild +
                        "`. Capital now has somewhere else to panic.";
                }
            }
        } else {
            message = "Usage: `/forex input: <markets|quote server_id [amount]|"
                      "exchange server_id amount>`";
            result = ECONOMY_INVALID_ARGUMENT;
        }
    } else if (action == "profile") {
        if (!input.empty() && input[0] == "global") {
            const size_t servers = player_server_count(user_id);
            const size_t collectibles = global_collectible_count(user_id);
            std::ostringstream out;
            out << "**Global Economic Identity**\n"
                << "Reputation: **" << global.reputation
                << "/100** | Connected economies: **" << servers << "**\n"
                << "Highest education: **"
                << kDegrees[std::min<int32_t>(
                    9, highest_education(global.education_mask))]
                << "** | License mask: **0x" << std::hex << std::uppercase
                << global.licenses << std::dec << "**\n"
                << "Global collectibles: **" << collectibles
                << "** | Achievements: **"
                << achievement_count(global.achievements) << "**\n"
                << "Lifetime actions: **" << global.lifetime_actions
                << "** | Cross-server FX: **"
                << currency_amount(
                    static_cast<int64_t>(std::min<uint64_t>(
                        global.lifetime_forex_cents,
                        static_cast<uint64_t>(kMaximumTransaction))),
                    settings.currency_symbol)
                << "**\nYour local balances and assets remain unique to each server.";
            message = out.str();
        } else {
            const int64_t liquid = wallet.wallet_cents + wallet.checking_cents +
                                   player.savings_cents + player.hysa_cents;
            const int64_t assets = portfolio_value(player, guild) +
                                   advanced_asset_value(advanced, guild) +
                                   property_equity(guild_id, user_id) +
                                   business_tangible_value(guild_id, user_id);
            std::ostringstream out;
            out << "**Global Player Identity**\n"
                << "Reputation: **" << global.reputation
                << "/100** | Connected economies: **"
                << player_server_count(user_id) << "**\n"
                << "Degree: **"
                << kDegrees[std::min<int32_t>(
                    9, highest_education(global.education_mask))]
                << "** (global) | Licenses: **0x" << std::hex
                << std::uppercase << global.licenses << std::dec << "**\n"
                << "Achievements: **"
                << achievement_count(global.achievements)
                << "** | Global collectibles: **"
                << global_collectible_count(user_id) << "**\n\n"
                << "**Current Server Economy**\n"
                << "Wallet: **" << cash(wallet.wallet_cents)
                << "** | Checking: **" << cash(wallet.checking_cents)
                << "**\nSavings/HYSA: **" << cash(player.savings_cents)
                << " / " << cash(player.hysa_cents)
                << "**\nDebt: **" << cash(player.debt_cents)
                << "** | Margin: **" << cash(advanced.margin_debt_cents)
                << "** | Credit: **" << player.credit_score
                << "**\nCareer (this server): **"
                << kJobs[std::min<uint32_t>(player.job_tier, 4)]
                << "**\nInvested assets: **" << cash(assets)
                << "** | Net worth: **"
                << cash(liquid + assets - player.debt_cents)
                << "**\nBankruptcies: **" << player.bankruptcies << "**";
            message = out.str();
        }
    } else if (action == "bank") {
        if (input.size() == 2 && input[0] == "select") {
            const int selected = std::atoi(input[1].c_str());
            if (selected < 1 || selected > static_cast<int>(kBanks.size())) {
                message = "Usage: `~bank select <1-6>`";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (bank_network.failed_until[selected - 1] > now) {
                message = "That bank is in resolution and cannot accept new customers.";
                result = ECONOMY_COOLDOWN;
            } else {
                lifecycle.bank_id = static_cast<uint32_t>(selected - 1);
                message = "Primary bank changed to **" +
                          std::string(kBanks[lifecycle.bank_id]) + "**.";
            }
        } else {
            std::ostringstream out;
            out << "**Local Banking Network**\n";
            for (size_t i = 0; i < kBanks.size(); ++i) {
                out << (i + 1) << ". " << kBanks[i] << " — APR "
                    << kBankAprBasisPoints[i] / 100.0 << "%, minimum credit "
                    << kBankMinimumCredit[i] << ", stability "
                    << bank_network.stability[i] << "/100";
                if (bank_network.failed_until[i] > now) {
                    out << " [RESOLUTION "
                        << (bank_network.failed_until[i] - now + 3599) / 3600
                        << "h]";
                }
                out << '\n';
            }
            out << "\nPrimary: **" << kBanks[std::min<uint32_t>(lifecycle.bank_id, 5)]
                << "** | Savings/HYSA: **" << cash(player.savings_cents) << " / "
                << cash(player.hysa_cents) << "**\nDebt: **" << cash(player.debt_cents)
                << "** | Credit: **" << player.credit_score << "** | Missed: **"
                << lifecycle.missed_payments << "** | Defaults: **" << lifecycle.defaults
                << "**\n`~bank select <1-6>`";
            if (lifecycle.next_payment_unix > now) {
                out << "\nNext minimum payment in **"
                    << (lifecycle.next_payment_unix - now) / 3600 << "h**.";
            }
            message = out.str();
        }
    } else if (action == "cd") {
        const std::string operation = input.empty() ? "status" : input[0];
        const size_t selected_bank =
            std::min<size_t>(lifecycle.bank_id, kBanks.size() - 1);
        if ((operation == "open" || operation == "close") &&
            bank_network.failed_until[selected_bank] > now) {
            message = "Certificate access is frozen while your bank is in resolution.";
            result = ECONOMY_COOLDOWN;
        } else if (operation == "open") {
            int64_t amount = 0;
            if (!parse_amount(1, amount) || advanced.cd_cents || !need_cash(amount)) {
                if (advanced.cd_cents) message = "Close the existing CD before opening another.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                wallet.wallet_cents -= amount;
                advanced.cd_cents = amount;
                advanced.cd_opened_unix = now;
                message = "Locked **" + cash(amount) + "** in a certificate of deposit.";
            }
        } else if (operation == "close") {
            if (!advanced.cd_cents) {
                message = "No CD exists.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                const bool matured = now - advanced.cd_opened_unix >= 24 * 3600;
                const int64_t payout = matured
                    ? advanced.cd_cents + advanced.cd_cents / 100
                    : advanced.cd_cents - advanced.cd_cents / 20;
                wallet.wallet_cents += payout;
                advanced.cd_cents = advanced.cd_opened_unix = 0;
                message = matured ? "Matured CD paid **" + cash(payout) + "**."
                                  : "Early withdrawal paid **" + cash(payout) +
                                    "** after the penalty.";
            }
        } else {
            message = "**Certificate of Deposit:** " + cash(advanced.cd_cents) +
                      "\n`~cd open <amount>` / `~cd close` (24-hour term)";
        }
    } else if (action == "invest") {
        if (input.size() < 3) {
            message = "Usage: `~invest <retirement|index|commodity> <buy|sell> <amount>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            int64_t amount = 0;
            int64_t* account = input[0] == "retirement" ? &advanced.retirement_cents :
                               input[0] == "index" ? &advanced.index_fund_cents :
                               input[0] == "commodity" ? &advanced.commodities_cents : nullptr;
            if (!account || !parse_amount(2, amount) ||
                (input[1] != "buy" && input[1] != "sell")) {
                message = "Usage: `~invest <retirement|index|commodity> <buy|sell> <amount>`";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (input[1] == "buy" && need_cash(amount)) {
                wallet.wallet_cents -= amount;
                *account += amount;
                message = "Invested **" + cash(amount) + "** into " + input[0] + ".";
            } else if (input[1] == "sell") {
                if (*account < amount) {
                    message = "That investment does not contain enough value.";
                    result = ECONOMY_INSUFFICIENT_FUNDS;
                } else {
                    *account -= amount;
                    const int64_t penalty = input[0] == "retirement" ? amount / 10 : 0;
                    wallet.wallet_cents += amount - penalty;
                    message = "Liquidated **" + cash(amount - penalty) +
                              "**" + (penalty ? " after retirement penalties." : ".");
                }
            }
        }
    } else if (action == "margin") {
        const std::string operation = input.empty() ? "status" : input[0];
        int64_t amount = 0;
        if (operation == "borrow" && parse_amount(1, amount)) {
            const int64_t limit = std::max<int64_t>(0, (player.credit_score - 650) * 10000LL);
            if (player.credit_score < 700 || advanced.margin_debt_cents + amount > limit) {
                message = "Margin denied. Requires **700** credit; limit: **" + cash(limit) + "**.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                advanced.margin_debt_cents += amount;
                wallet.wallet_cents += amount;
                message = "Borrowed **" + cash(amount) + "** on margin. Wonderful judgment.";
            }
        } else if (operation == "repay" && parse_amount(1, amount) && need_cash(amount)) {
            amount = std::min(amount, advanced.margin_debt_cents);
            wallet.wallet_cents -= amount;
            advanced.margin_debt_cents -= amount;
            message = "Repaid **" + cash(amount) + "** of margin debt.";
        } else {
            message = "**Margin debt:** " + cash(advanced.margin_debt_cents) +
                      "\n`~margin <borrow|repay> <amount>`";
        }
    } else if (action == "savings" || action == "hysa") {
        int64_t amount = 0;
        if (input.size() < 2 || !parse_amount(1, amount) ||
            (input[0] != "deposit" && input[0] != "withdraw")) {
            message = "Usage: `~" + action + " <deposit|withdraw> <amount>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            int64_t& account = action == "hysa" ? player.hysa_cents : player.savings_cents;
            const size_t selected_bank =
                std::min<size_t>(lifecycle.bank_id, kBanks.size() - 1);
            if (bank_network.failed_until[selected_bank] > now) {
                message = "Deposit access is frozen during resolution; insured balances "
                          "remain protected.";
                result = ECONOMY_COOLDOWN;
            } else if (input[0] == "deposit" && need_cash(amount)) {
                wallet.wallet_cents -= amount;
                account += amount;
                message = "Moved **" + cash(amount) + "** into " + action + ".";
            } else if (input[0] == "withdraw") {
                if (account < amount) {
                    message = "That account does not contain enough money.";
                    result = ECONOMY_INSUFFICIENT_FUNDS;
                } else {
                    account -= amount;
                    wallet.wallet_cents += amount;
                    message = "Withdrew **" + cash(amount) + "** from " + action + ".";
                }
            }
        }
    } else if (action == "loan") {
        int64_t amount = 0;
        const size_t bank = std::min<size_t>(lifecycle.bank_id, kBanks.size() - 1);
        const int64_t base_limit =
            std::max<int64_t>(5000, (player.credit_score - 300) * 5000LL);
        const int64_t limit = base_limit * kBankLoanLimitPercent[bank] / 100;
        if (parse_amount(0, amount)) {
            if (bank_network.failed_until[bank] > now) {
                message = std::string(kBanks[bank]) +
                          " is in resolution and has suspended lending.";
                result = ECONOMY_COOLDOWN;
            } else if (player.credit_score < kBankMinimumCredit[bank] ||
                player.debt_cents + amount > limit) {
                message = std::string(kBanks[bank]) +
                          " denied the loan. Current borrowing limit: **" + cash(limit) +
                          "**; credit score: **" + std::to_string(player.credit_score) + "**.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                wallet.wallet_cents += amount;
                const int64_t interest = amount * kBankAprBasisPoints[bank] / 10000;
                player.debt_cents += amount + interest;
                if (!lifecycle.next_payment_unix) lifecycle.next_payment_unix = now + 24 * 3600;
                player.credit_score = std::max(300, player.credit_score - 8);
                message = std::string(kBanks[bank]) + " approved **" + cash(amount) +
                          "**. Total added debt: **" + cash(amount + interest) +
                          "**; first minimum payment is due in 24 hours.";
            }
        }
    } else if (action == "card") {
        const std::string operation = input.empty() ? "status" : input[0];
        int64_t amount = 0;
        if (operation == "charge" && parse_amount(1, amount)) {
            const int64_t limit = std::max<int64_t>(10000, (player.credit_score - 450) * 2500LL);
            if (player.credit_score < 550 || player.debt_cents + amount > limit) {
                message = "Card declined. Limit: **" + cash(limit) +
                          "** | Credit: **" + std::to_string(player.credit_score) + "**.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                player.debt_cents += amount;
                if (!lifecycle.next_payment_unix) lifecycle.next_payment_unix = now + 24 * 3600;
                wallet.wallet_cents += amount + amount / 100;
                message = "Charged **" + cash(amount) + "** and earned **" +
                          cash(amount / 100) + "** cash back. Debt is thriving.";
            }
        } else {
            message = "**Credit card balance:** " + cash(player.debt_cents) +
                      "\n`~card charge <amount>`; repay with `~repay <amount>`.";
        }
    } else if (action == "repay") {
        int64_t amount = 0;
        if (parse_amount(0, amount) && need_cash(amount)) {
            amount = std::min(amount, player.debt_cents);
            wallet.wallet_cents -= amount;
            player.debt_cents -= amount;
            if (player.debt_cents <= 0) {
                player.debt_cents = 0;
                lifecycle.next_payment_unix = 0;
                lifecycle.missed_payments = 0;
            } else if (amount >= std::max<int64_t>(500, player.debt_cents / 20)) {
                lifecycle.next_payment_unix = now + 24 * 3600;
                lifecycle.missed_payments = 0;
            }
            player.credit_score = std::min(850, player.credit_score + 5 + int(amount / 10000));
            message = "Repaid **" + cash(amount) + "**. Credit score: **" +
                      std::to_string(player.credit_score) + "**.";
        }
    } else if (action == "bankruptcy") {
        if (player.debt_cents < 10000) {
            message = "Bankruptcy court refuses to process this tiny tragedy.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            const int64_t discharged = player.debt_cents;
            player.debt_cents = 0;
            lifecycle.next_payment_unix = 0;
            lifecycle.missed_payments = 0;
            player.savings_cents = player.hysa_cents = player.bonds_cents = 0;
            player.shares.fill(0);
            player.business_cash_cents = 0;
            player.business_level = 0;
            player.property_level = 0;
            business = BusinessProfile{};
            advanced = AdvancedPlayer{};
            g_orders.erase(std::remove_if(g_orders.begin(), g_orders.end(),
                [&](const LimitOrder& order) {
                    return order.guild_id == guild_id && order.user_id == user_id;
                }), g_orders.end());
            for (auto auction = g_auctions.begin(); auction != g_auctions.end();) {
                if (auction->guild_id != guild_id) {
                    ++auction;
                } else if (auction->seller_id == user_id) {
                    if (!auction->highest_bidder_id.empty()) {
                        ensure_player(guild_id, auction->highest_bidder_id).wallet_cents +=
                            auction->highest_bid_cents;
                    }
                    auction = g_auctions.erase(auction);
                } else if (auction->highest_bidder_id == user_id) {
                    auction->highest_bidder_id.clear();
                    auction->highest_bid_cents = 0;
                    ++auction;
                } else {
                    ++auction;
                }
            }
            g_properties.erase(std::remove_if(g_properties.begin(), g_properties.end(),
                [&](const PropertyAsset& property) {
                    return property.guild_id == guild_id && property.owner_id == user_id;
                }), g_properties.end());
            g_partnerships.erase(std::remove_if(
                g_partnerships.begin(), g_partnerships.end(),
                [&](const BusinessPartnership& partnership) {
                    return partnership.guild_id == guild_id &&
                           (partnership.owner_id == user_id ||
                           partnership.partner_id == user_id);
                }), g_partnerships.end());
            g_collectible_assets.erase(std::remove_if(
                g_collectible_assets.begin(), g_collectible_assets.end(),
                [&](const CollectibleAsset& collectible) {
                    return collectible.guild_id == guild_id &&
                           collectible.owner_id == user_id;
                }), g_collectible_assets.end());
            for (ScheduledAgreement& agreement : g_agreements) {
                if (agreement.guild_id == guild_id &&
                    (agreement.issuer_id == user_id ||
                     agreement.counterparty_id == user_id)) {
                    agreement.active = false;
                    agreement.remaining_payments = 0;
                }
            }
            wallet.checking_cents = 0;
            wallet.wallet_cents = std::min<int64_t>(wallet.wallet_cents, 5000);
            player.credit_score = 300;
            ++player.bankruptcies;
            message = "Bankruptcy discharged **" + cash(discharged) +
                      "**. Assets were seized, credit imploded, and basic work remains available.";
        }
    } else if (action == "career") {
        message = "**Career Ladder**\n1. Fast-food Survivor — no requirements\n"
            "2. Skilled Technician — 10 experience\n3. Financial Analyst — degree + 30 experience\n"
            "4. Executive — Business/Finance degree + 80 experience\nCurrent: **" +
            std::string(kJobs[std::min<uint32_t>(player.job_tier, 4)]) +
            "** | Experience: **" + std::to_string(player.experience) +
            "** | Performance: **" + std::to_string(lifecycle.performance) +
            "/100**\nLayoffs survived: **" + std::to_string(lifecycle.layoffs) +
            "** | Retirement benefits: **" + cash(advanced.retirement_cents) +
            "**\nStrong reviews can trigger automatic promotions. Apply: `~applyjob <1-4>`";
    } else if (action == "certifications") {
        static constexpr std::array<const char*, 5> slugs = {
            "bookkeeping", "trader", "technician", "manager", "realtor"
        };
        static constexpr std::array<const char*, 5> names = {
            "Certified Bookkeeper", "Licensed Market Trader",
            "Industrial Technician", "Management Professional",
            "Real Estate Specialist"
        };
        static constexpr std::array<int64_t, 5> costs = {
            50000, 75000, 60000, 80000, 70000
        };
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "buy") {
            size_t certification = slugs.size();
            if (input.size() > 1) {
                for (size_t i = 0; i < slugs.size(); ++i) {
                    if (input[1] == slugs[i]) certification = i;
                }
            }
            const bool requirement =
                certification == 0 ? player.experience >= 5 || player.degree == 3 :
                certification == 1 ? player.experience >= 20 ||
                    player.degree == 1 || player.degree == 2 :
                certification == 2 ? player.experience >= 15 ||
                    player.degree == 4 || player.degree == 5 :
                certification == 3 ? player.experience >= 30 ||
                    player.degree == 6 || player.degree == 9 :
                certification == 4 ? property_equity(guild_id, user_id) > 0 : false;
            if (certification >= slugs.size() ||
                (development.certifications & (1u << certification)) ||
                !requirement ||
                (certification < slugs.size() && !need_cash(costs[certification]))) {
                if (message.empty()) {
                    message = "Certification rejected: verify prerequisites, ownership, "
                              "and tuition. Use `~certifications`.";
                }
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                wallet.wallet_cents -= costs[certification];
                development.certifications |= 1u << certification;
                player.credit_score = std::min(850, player.credit_score + 3);
                message = "Earned **" + std::string(names[certification]) +
                          "** for **" + cash(costs[certification]) + "**.";
            }
        } else {
            std::ostringstream out;
            out << "**Professional Certifications**\n";
            for (size_t i = 0; i < names.size(); ++i) {
                out << (development.certifications & (1u << i) ? "✅ " : "▫️ ")
                    << names[i] << " — " << cash(costs[i])
                    << " — `~certifications buy " << slugs[i] << "`\n";
            }
            out << "Prerequisites use experience, relevant degrees, or property ownership.";
            message = out.str();
        }
    } else if (action == "skills") {
        const int degree_bonus = player.degree ? 15 : 0;
        const int financial = std::clamp<int>(
            player.experience / 3 + degree_bonus +
            (player.degree == 1 || player.degree == 2 || player.degree == 3 ? 25 : 0),
            0, 100);
        const int technical = std::clamp<int>(
            player.experience / 4 + degree_bonus +
            (player.degree == 4 || player.degree == 5 ? 30 : 0) +
            ((development.certifications & (1u << 2)) ? 15 : 0), 0, 100);
        const int management = std::clamp<int>(
            player.experience / 4 + business.reputation / 3 +
            (player.degree == 6 ? 25 : 0) +
            ((development.certifications & (1u << 3)) ? 15 : 0), 0, 100);
        const int negotiation = std::clamp<int>(
            player.experience / 5 + (player.degree == 9 ? 35 : 0) +
            business.marketing / 3, 0, 100);
        const int risk = std::clamp<int>(
            financial / 2 + (player.degree == 1 ? 25 : 0) +
            ((development.certifications & (1u << 1)) ? 20 : 0), 0, 100);
        message = "**Derived Skills**\nFinancial literacy: **" +
            std::to_string(financial) + "/100** | Risk assessment: **" +
            std::to_string(risk) + "/100**\nTechnical: **" +
            std::to_string(technical) + "/100** | Management: **" +
            std::to_string(management) + "/100**\nNegotiation: **" +
            std::to_string(negotiation) +
            "/100**\nSkills improve information, efficiency, and access; they never "
            "guarantee outcomes.";
    } else if (action == "applyjob") {
        uint32_t tier = input.empty() ? 0 : static_cast<uint32_t>(std::atoi(input[0].c_str()));
        const uint32_t xp_required[] = {0, 0, 10, 30, 80};
        if (tier < 1 || tier > 4 || player.experience < xp_required[tier] ||
            (tier >= 3 && player.degree == 0) ||
            (tier == 4 && player.degree != 1 && player.degree != 6)) {
            message = "Application rejected. Check `~career` requirements.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            player.job_tier = tier;
            lifecycle.performance = std::max(55, lifecycle.performance);
            lifecycle.last_review_unix = now;
            player.credit_score = std::min(850, player.credit_score + int(tier * 3));
            message = "Hired as **" + std::string(kJobs[tier]) + "**.";
        }
    } else if (action == "college") {
        message = "**Colleges and Degrees**\nMiyamii Institute, Princeton, Harvard, Wheat University,\n"
            "Shrimp CC, Sea Slug CC, Rivamonte, Florensa, and Sielgrada\n"
            "Degrees: `1 Finance`, `2 Economics`, `3 Accounting`, `4 Computer Science`,\n"
            "`5 Engineering`, `6 Business`, `7 Marketing`, `8 Law`, `9 Psychology`\n"
            "Tuition: **" + cash(150000) + "** — `~enroll <1-9>`";
    } else if (action == "enroll") {
        uint32_t degree = input.empty() ? 0 : static_cast<uint32_t>(std::atoi(input[0].c_str()));
        if (degree < 1 || degree > 9 || !need_cash(150000)) {
            if (degree < 1 || degree > 9) message = "Usage: `~enroll <degree 1-9>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            wallet.wallet_cents -= 150000;
            player.degree = degree;
            player.credit_score = std::min(850, player.credit_score + 15);
            message = "Graduated in **" + std::string(kDegrees[degree]) +
                      "**. Somehow the diploma was instantaneous.";
        }
    } else if (action == "market") {
        std::ostringstream out;
        out << "**Routine Local Exchange**\n";
        for (size_t i = 0; i < kStockCount; ++i) {
            const int64_t spread = stock_spread(exchange, i, guild.prices[i]);
            const int64_t bid = std::max<int64_t>(1, guild.prices[i] - spread / 2);
            const int64_t ask = guild.prices[i] + (spread + 1) / 2;
            const std::string status = !exchange.listed[i] ? " DELISTED" :
                exchange.halted_until[i] > now ? " HALTED" : "";
            out << '`' << kTickers[i] << "` " << cash(bid) << " / "
                << cash(ask) << " • vol " << exchange.volume[i]
                << status << '\n';
        }
        out << "Bid / ask shown. `~fundamentals <ticker>` "
               "`~stock <buy|sell> <ticker> <shares>`";
        message = out.str();
    } else if (action == "fundamentals") {
        const size_t index = !input.empty() ? stock_index(input[0]) : kStockCount;
        if (index == kStockCount) {
            message = "Usage: `~fundamentals <ticker>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            const int64_t spread = stock_spread(exchange, index, guild.prices[index]);
            const int64_t bid = std::max<int64_t>(
                1, guild.prices[index] - spread / 2);
            const int64_t ask = guild.prices[index] + (spread + 1) / 2;
            const int64_t market_cap = guild.prices[index] *
                static_cast<int64_t>(std::min<uint64_t>(
                    exchange.shares_outstanding[index],
                    static_cast<uint64_t>(
                        std::numeric_limits<int64_t>::max() /
                        std::max<int64_t>(1, guild.prices[index]))));
            message = "**" + std::string(kTickers[index]) +
                " · " + kCompanyNames[index] +
                "**\nPersonality: *" + kCompanyPersonalities[index] +
                "*\nBid / ask: **" + cash(bid) + " / " +
                cash(ask) + "** | Midpoint: **" + cash(guild.prices[index]) +
                "**\nEstimated fundamental value: **" +
                cash(stock_fundamental_value(exchange, index)) +
                "** | Sentiment: **" + std::to_string(exchange.sentiment[index]) +
                "/100**\nRevenue / expenses / profit: **" +
                cash(exchange.revenue[index]) + " / " +
                cash(exchange.expenses[index]) + " / " +
                cash(exchange.profit[index]) + "**\nCash / debt: **" +
                cash(exchange.cash[index]) + " / " +
                cash(exchange.debt[index]) + "**\nShares outstanding: **" +
                std::to_string(exchange.shares_outstanding[index]) +
                "** | Market cap: **" + cash(market_cap) +
                "**\nRolling volume: **" + std::to_string(exchange.volume[index]) +
                "** | Hourly liquidity: **" +
                std::to_string(exchange.liquidity[index]) +
                "** | Distress: **" + std::to_string(exchange.distress[index]) +
                "/24** | Expectations: **" +
                (dynamics.expectations[index] >= 0 ? "+" : "") +
                std::to_string(dynamics.expectations[index]) +
                "**\nStatus: **" +
                (!exchange.listed[index] ? "DELISTED / RESTRUCTURING" :
                 exchange.halted_until[index] > now ? "TRADING HALT" : "OPEN") + "**";
        }
    } else if (action == "stock") {
        const size_t index = input.size() > 1 ? stock_index(input[1]) : kStockCount;
        uint32_t quantity = input.size() > 2
            ? static_cast<uint32_t>(std::strtoul(input[2].c_str(), nullptr, 10)) : 0;
        if (input.size() < 3 || index == kStockCount || quantity == 0 || quantity > 100000 ||
            (input[0] != "buy" && input[0] != "sell")) {
            message = "Usage: `~stock <buy|sell> <ticker> <shares>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            if (!exchange.listed[index]) {
                message = "That company is delisted and undergoing restructuring.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (exchange.halted_until[index] > now) {
                message = "Trading is halted in **" + std::string(kTickers[index]) +
                          "** after an extreme move.";
                result = ECONOMY_COOLDOWN;
            } else if (!exchange.remaining_liquidity[index]) {
                message = "Market makers exhausted this hour's liquidity. Use a limit order.";
                result = ECONOMY_COOLDOWN;
            } else {
            const uint32_t fill = std::min(
                quantity, exchange.remaining_liquidity[index]);
            const int64_t spread = stock_spread(exchange, index, guild.prices[index]);
            const int64_t execution = input[0] == "buy"
                ? guild.prices[index] + (spread + 1) / 2
                : std::max<int64_t>(1, guild.prices[index] - spread / 2);
            const int64_t value = execution * fill;
            const bool licensed_trader = development.certifications & (1u << 1);
            const int64_t fee = licensed_trader
                ? std::max<int64_t>(15, value / 750)
                : std::max<int64_t>(25, value / 500);
            if (input[0] == "buy" && need_cash(value + fee)) {
                if (player.shares[index] > std::numeric_limits<uint32_t>::max() - fill) {
                    message = "That position exceeds the exchange ownership limit.";
                    result = ECONOMY_INVALID_ARGUMENT;
                } else {
                    wallet.wallet_cents -= value + fee;
                    player.shares[index] += fill;
                    exchange.remaining_liquidity[index] -= fill;
                    exchange.volume[index] += fill;
                    guild.prices[index] = std::min<int64_t>(1000000000,
                        guild.prices[index] + std::max<int64_t>(
                            1, guild.prices[index] * fill /
                                std::max<uint32_t>(
                                    1000, exchange.liquidity[index] * 20)));
                    message = "Bought **" + std::to_string(fill) + "/" +
                        std::to_string(quantity) + " " + kTickers[index] +
                        "** at **" + cash(execution) + "** for **" +
                        cash(value + fee) + "** including fees." +
                        (fill < quantity ? " The market order partially filled." : "");
                }
            } else if (input[0] == "sell") {
                if (player.shares[index] < quantity) {
                    message = "You do not own that many shares.";
                    result = ECONOMY_INSUFFICIENT_FUNDS;
                } else {
                    player.shares[index] -= fill;
                    wallet.wallet_cents += value - fee;
                    exchange.remaining_liquidity[index] -= fill;
                    exchange.volume[index] += fill;
                    guild.prices[index] = std::max<int64_t>(50,
                        guild.prices[index] - std::max<int64_t>(
                            1, guild.prices[index] * fill /
                                std::max<uint32_t>(
                                    1000, exchange.liquidity[index] * 20)));
                    message = "Sold **" + std::to_string(fill) + "/" +
                        std::to_string(quantity) + " " + kTickers[index] +
                        "** at **" + cash(execution) + "** for **" +
                        cash(value - fee) + "** after fees." +
                        (fill < quantity ? " The market order partially filled." : "");
                }
            }
            }
        }
    } else if (action == "derivatives") {
        const size_t index = input.size() > 1 ? stock_index(input[1]) : kStockCount;
        int64_t premium = 0;
        if (input.size() < 3 || (input[0] != "call" && input[0] != "put") ||
            index == kStockCount || !parse_cash(input[2], premium)) {
            message = "Usage: `~derivatives <call|put> <ticker> <premium>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else if (!exchange.listed[index] || exchange.halted_until[index] > now) {
            message = "Derivatives cannot price a halted or delisted company.";
            result = ECONOMY_COOLDOWN;
        } else if (player.degree != 1 && player.credit_score < 650) {
            message = "Options access requires a Finance degree or **650** credit.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else if (need_cash(premium)) {
            wallet.wallet_cents -= premium;
            std::uniform_int_distribution<int> move(-100, 100);
            const int simulated_move = move(g_rng) + (guild.confidence - 50) / 2;
            const bool won = (input[0] == "call" && simulated_move > 18) ||
                             (input[0] == "put" && simulated_move < -18);
            const int64_t payout = won ? premium * 3 : 0;
            wallet.wallet_cents += payout;
            message = won ? "The " + input[0] + " on **" + kTickers[index] +
                              "** paid **" + cash(payout) + "**."
                          : "The option expired worthless. **" + cash(premium) +
                              "** achieved nonexistence.";
        }
    } else if (action == "orders") {
        const std::string operation = input.empty() ? "list" : input[0];
        if (operation == "place") {
            const size_t index = input.size() > 2 ? stock_index(input[2]) : kStockCount;
            const bool buy = input.size() > 1 && input[1] == "buy";
            const bool sell = input.size() > 1 && input[1] == "sell";
            uint32_t quantity = input.size() > 3
                ? static_cast<uint32_t>(std::strtoul(input[3].c_str(), nullptr, 10)) : 0;
            int64_t limit = 0;
            const size_t open_orders = static_cast<size_t>(std::count_if(
                g_orders.begin(), g_orders.end(), [&](const LimitOrder& order) {
                    return order.guild_id == guild_id && order.user_id == user_id;
                }));
            const size_t order_cap =
                (development.certifications & (1u << 1)) ? 50 : 25;
            if ((!buy && !sell) || index == kStockCount || quantity == 0 ||
                quantity > 100000 || !parse_amount(4, limit) ||
                limit > kMaximumTransaction / quantity) {
                message = "Usage: `~orders place <buy|sell> <ticker> <shares> <limit-price>`";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (open_orders >= order_cap) {
                message = "Open-order cap reached (**" +
                    std::to_string(order_cap) + "**). Cancel an order first.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (!exchange.listed[index]) {
                message = "The exchange rejects new orders in a delisted company.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (buy) {
                const int64_t reserve = limit * quantity;
                if (need_cash(reserve)) {
                    wallet.wallet_cents -= reserve;
                    g_orders.push_back({g_next_order_id++, guild_id, user_id,
                                        static_cast<uint32_t>(index), quantity, limit, true});
                    message = "Placed buy order **#" + std::to_string(g_orders.back().id) +
                              "** with **" + cash(reserve) + "** reserved.";
                }
            } else if (player.shares[index] < quantity) {
                message = "Insufficient shares to reserve for that sell order.";
                result = ECONOMY_INSUFFICIENT_FUNDS;
            } else {
                player.shares[index] -= quantity;
                g_orders.push_back({g_next_order_id++, guild_id, user_id,
                                    static_cast<uint32_t>(index), quantity, limit, false});
                message = "Placed sell order **#" + std::to_string(g_orders.back().id) + "**.";
            }
        } else if (operation == "cancel") {
            const uint64_t id = input.size() > 1 ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto order = std::find_if(g_orders.begin(), g_orders.end(),
                [&](const LimitOrder& candidate) {
                    return candidate.id == id && candidate.guild_id == guild_id &&
                           candidate.user_id == user_id;
                });
            if (order == g_orders.end()) {
                message = "Order not found.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                if (order->buy) {
                    wallet.wallet_cents += order->limit_price_cents * order->quantity;
                } else {
                    player.shares[order->stock] += order->quantity;
                }
                g_orders.erase(order);
                message = "Cancelled order **#" + std::to_string(id) + "** and released escrow.";
            }
        } else {
            std::ostringstream out;
            out << "**Open Orders**\n";
            size_t count = 0;
            for (const LimitOrder& order : g_orders) {
                if (order.guild_id == guild_id && order.user_id == user_id) {
                    ++count;
                    out << '#' << order.id << ' ' << (order.buy ? "BUY " : "SELL ")
                        << kTickers[order.stock] << " ×" << order.quantity << " @ "
                        << cash(order.limit_price_cents) << '\n';
                }
            }
            if (!count) out << "None.\n";
            out << "`~orders place ...` / `~orders cancel <id>`";
            message = out.str();
        }
    } else if (action == "short") {
        const std::string operation = input.empty() ? "status" : input[0];
        const size_t index = input.size() > 1 ? stock_index(input[1]) : kStockCount;
        uint32_t quantity = input.size() > 2
            ? static_cast<uint32_t>(std::strtoul(input[2].c_str(), nullptr, 10)) : 0;
        if ((operation != "open" && operation != "close") ||
            index == kStockCount || quantity == 0 || quantity > 100000) {
            std::ostringstream out;
            out << "**Short Positions**\n";
            for (size_t i = 0; i < kStockCount; ++i) if (advanced.short_shares[i]) {
                out << kTickers[i] << ": " << advanced.short_shares[i] << " shares\n";
            }
            out << "`~short <open|close> <ticker> <shares>`";
            message = out.str();
        } else if (!exchange.listed[index] || exchange.halted_until[index] > now) {
            message = "Short trading is unavailable while the company is halted or delisted.";
            result = ECONOMY_COOLDOWN;
        } else if (player.credit_score < 680 && player.degree != 1) {
            message = "Shorting requires **680** credit or a Finance degree.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else if (operation == "open") {
            if (advanced.short_shares[index] >
                std::numeric_limits<uint32_t>::max() - quantity) {
                message = "Short-position limit exceeded.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                advanced.short_shares[index] += quantity;
                const int64_t proceeds = guild.prices[index] * quantity;
                wallet.wallet_cents += proceeds;
                message = "Shorted **" + std::to_string(quantity) + " " + kTickers[index] +
                          "** and received **" + cash(proceeds) + "**. Liability remains.";
            }
        } else if (advanced.short_shares[index] < quantity) {
            message = "You do not have that many shares short.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            const int64_t cover = guild.prices[index] * quantity;
            advanced.short_shares[index] -= quantity;
            if (wallet.wallet_cents >= cover) {
                wallet.wallet_cents -= cover;
            } else {
                advanced.margin_debt_cents += cover - wallet.wallet_cents;
                wallet.wallet_cents = 0;
            }
            message = "Covered **" + std::to_string(quantity) + " " + kTickers[index] +
                      "** for **" + cash(cover) + "**.";
        }
    } else if (action == "portfolio") {
        std::ostringstream out;
        out << "**Portfolio**\nBonds: **" << cash(player.bonds_cents)
            << "** | Index: **" << cash(advanced.index_fund_cents)
            << "** | Retirement: **" << cash(advanced.retirement_cents)
            << "**\nBusiness cash: **" << cash(player.business_cash_cents)
            << "** | Commodities: **" << cash(advanced.commodities_cents) << "**\n";
        bool any = false;
        for (size_t i = 0; i < kStockCount; ++i) if (player.shares[i]) {
            any = true;
            out << kTickers[i] << ": **" << player.shares[i] << "** ("
                << cash(player.shares[i] * guild.prices[i]) << ")\n";
        }
        if (!any) out << "No stocks. Your risk-adjusted peace is magnificent.\n";
        out << "Property equity: **" << cash(property_equity(guild_id, user_id))
            << "** | Company assets: **"
            << cash(business_tangible_value(guild_id, user_id))
            << "**\nTotal invested assets: **"
            << cash(portfolio_value(player, guild) + advanced_asset_value(advanced, guild) +
                    property_equity(guild_id, user_id) +
                    business_tangible_value(guild_id, user_id))
            << "**";
        message = out.str();
    } else if (action == "bonds") {
        int64_t amount = 0;
        if (input.size() < 2 || !parse_amount(1, amount) ||
            (input[0] != "buy" && input[0] != "sell")) {
            message = "Usage: `~bonds <buy|sell> <amount>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else if (input[0] == "buy" && need_cash(amount)) {
            wallet.wallet_cents -= amount;
            player.bonds_cents += amount;
            message = "Bought **" + cash(amount) + "** in government bonds.";
        } else if (input[0] == "sell") {
            if (player.bonds_cents < amount) {
                message = "You do not own that many bonds.";
                result = ECONOMY_INSUFFICIENT_FUNDS;
            } else {
                player.bonds_cents -= amount;
                const int64_t proceeds = amount;
                wallet.wallet_cents += proceeds;
                message = "Redeemed bonds for **" + cash(proceeds) + "**.";
            }
        }
    } else if (action == "business") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "start") {
            const uint32_t chosen_industry = input.size() > 1
                ? industry_index(input[1]) : 5;
            if (player.business_level || player.credit_score < 580 || !need_cash(100000)) {
                if (!player.business_level) message =
                    "Starting requires **" + cash(100000) + "** and a **580** credit score.";
                else message = "You already own a business.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (!chosen_industry) {
                message = "Choose `food`, `manufacturing`, `tech`, `logistics`, or `retail`.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                wallet.wallet_cents -= 100000;
                player.business_cash_cents = 100000;
                player.business_level = 1;
                business = BusinessProfile{};
                business.industry = chosen_industry;
                business.last_decay_unix = now;
                guild.confidence = std::min(90, guild.confidence + 1);
                guild.unemployment_bp = std::max(150, guild.unemployment_bp - 10);
                message = "A **" + std::string(kIndustries[chosen_industry]) +
                    "** business was licensed. Payroll immediately developed opinions.";
            }
        } else if (operation == "operate") {
            if (!player.business_level) {
                message = "Start one with `~business start`.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (player.last_business_unix &&
                       now - player.last_business_unix < 4 * 3600) {
                message = "Customers need time to regenerate. Operate every four hours.";
                result = ECONOMY_COOLDOWN;
            } else {
                const uint32_t industry = std::clamp<uint32_t>(business.industry, 1, 5);
                const uint32_t demand_capacity = std::max<uint32_t>(
                    1, player.business_level * 3 + business.equipment_level * 2 +
                    business.marketing / 10 +
                    static_cast<uint32_t>(std::max(0, business.reputation)) / 10);
                const uint32_t sold = std::min(business.finished_goods, demand_capacity);
                int64_t sector_pressure = 0;
                if (industry == 1) {
                    sector_pressure = (50 - guild.confidence) / 3;
                } else if (industry == 2) {
                    sector_pressure = (guild.confidence - 45) / 2 -
                                      guild.inflation_bp / 300;
                } else if (industry == 3) {
                    sector_pressure = (guild.confidence - 50) * 2 / 3;
                } else if (industry == 4) {
                    sector_pressure = guild.inflation_bp / 250 -
                                      guild.unemployment_bp / 500;
                } else if (industry == 5) {
                    sector_pressure = guild.confidence - 50;
                }
                const int64_t demand_index = std::clamp<int64_t>(
                    40 + guild.confidence + business.marketing / 2 +
                    std::max(0, business.reputation) / 2 + sector_pressure,
                    60, 190);
                int64_t gross = static_cast<int64_t>(sold) *
                    kIndustrySalePrice[industry] * demand_index / 100;
                if (!sold) {
                    std::uniform_int_distribution<int64_t> service_revenue(1000, 4000);
                    gross = service_revenue(g_rng) * player.business_level *
                        (50 + guild.confidence) / 100;
                }
                if (!government.mayor_id.empty() && government.platform == 2 &&
                    government.term_end_unix > now) {
                    gross += gross / 10;
                }
                business.finished_goods -= sold;
                const int64_t certification_efficiency =
                    (development.certifications & (1u << 0) ? 2 : 0) +
                    (development.certifications & (1u << 3) ? 2 : 0);
                const int64_t expense_rate = std::max<int64_t>(
                    18, 42 - std::min<uint32_t>(12, player.degree * 2) -
                    business.equipment_level * 2 - certification_efficiency);
                const int64_t expenses =
                    gross * expense_rate / 100 +
                    static_cast<int64_t>(advanced.employees) *
                        (player.degree == 9 ? 1800 : 2000);
                const int64_t profit = gross - expenses;
                player.business_cash_cents += profit;
                player.last_business_unix = now;
                business.lifetime_revenue_cents += gross;
                business.lifetime_profit_cents += profit;
                business.reputation = std::clamp(
                    business.reputation + (sold ? profit >= 0 ? 2 : -3 : -1), 0, 100);
                std::string partner_id;
                const uint32_t partner_share =
                    active_partner_share(guild_id, user_id, &partner_id);
                int64_t partner_distribution = 0;
                if (profit > 0 && partner_share && !partner_id.empty()) {
                    partner_distribution = std::min(
                        player.business_cash_cents,
                        profit * partner_share / 10000);
                    player.business_cash_cents -= partner_distribution;
                    ensure_player(guild_id, partner_id).wallet_cents +=
                        partner_distribution;
                }
                if (player.business_cash_cents < 0) {
                    player.debt_cents += -player.business_cash_cents;
                    player.debt_cents += business.debt_cents;
                    player.business_cash_cents = 0;
                    player.business_level = 0;
                    advanced.employees = advanced.business_inventory = 0;
                    const uint32_t prior_defaults = business.defaults;
                    business = BusinessProfile{};
                    business.defaults = prior_defaults;
                    business.reputation = 10;
                    g_partnerships.erase(std::remove_if(
                        g_partnerships.begin(), g_partnerships.end(),
                        [&](const BusinessPartnership& partnership) {
                            return partnership.guild_id == guild_id &&
                                   partnership.owner_id == user_id;
                        }), g_partnerships.end());
                    guild.confidence = std::max(10, guild.confidence - 3);
                    guild.unemployment_bp = std::min(3000, guild.unemployment_bp + 50);
                    message = "Revenue failed to cover payroll. The business collapsed and "
                              "its deficit became personal debt.";
                } else {
                    message = "Revenue **" + cash(gross) + "** − expenses **" +
                              cash(expenses) + "** = profit **" + cash(profit) +
                              "**. Sold **" + std::to_string(sold) +
                              "** unit(s); reputation **" +
                              std::to_string(business.reputation) + "/100**.";
                    if (partner_distribution) {
                        message += " Partner distribution: **" +
                                   cash(partner_distribution) + "**.";
                    }
                }
            }
        } else if (operation == "export") {
            const std::string target_guild =
                input.size() > 1 ? input[1] : "";
            const uint32_t quantity = input.size() > 2
                ? static_cast<uint32_t>(
                    std::strtoul(input[2].c_str(), nullptr, 10)) : 0;
            if (!player.business_level || !valid_id(target_guild.c_str()) ||
                target_guild == guild_id || !quantity || quantity > 1000 ||
                business.finished_goods < quantity ||
                g_guilds.find(target_guild) == g_guilds.end()) {
                message = "Usage: `/business input: export <server_id> <1-1000>` "
                          "with enough finished goods and a connected target economy.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                GuildDynamics& target_dynamics = g_dynamics[target_guild];
                const GuildEconomy& target_economy = g_guilds[target_guild];
                cross_target_guild = target_guild;
                cross_target_dynamics_before = target_dynamics;
                cross_target_dynamics_changed = true;
                const uint32_t industry =
                    std::clamp<uint32_t>(business.industry, 1, 5);
                const int32_t treaty_discount =
                    dynamics.trade_partner == target_guild &&
                    target_dynamics.trade_partner == guild_id ? 2 : 1;
                const int32_t tariff = target_dynamics.tariff_basis_points /
                    treaty_discount;
                const int64_t gross =
                    static_cast<int64_t>(quantity) *
                    kIndustrySalePrice[industry] *
                    std::clamp(target_economy.confidence, 20, 90) / 55;
                const int64_t tariff_cost = gross * tariff / 10000;
                const int64_t shipping = std::max<int64_t>(100, gross / 10);
                const int64_t proceeds =
                    std::max<int64_t>(0, gross - tariff_cost - shipping);
                business.finished_goods -= quantity;
                player.business_cash_cents += proceeds;
                business.lifetime_revenue_cents += gross;
                business.lifetime_profit_cents += proceeds;
                add_bounded(dynamics.exports_cents, gross);
                add_bounded(dynamics.capital_inflow_cents, proceeds);
                add_bounded(target_dynamics.imports_cents, gross);
                add_bounded(target_dynamics.capital_outflow_cents, proceeds);
                global.lifetime_trade_cents += static_cast<uint64_t>(gross);
                global.reputation = std::min(100, global.reputation + 1);
                message = "Exported **" + std::to_string(quantity) +
                    "** unit(s) to server `" + target_guild +
                    "`.\nGross: **" + cash(gross) +
                    "** | tariff: **" + cash(tariff_cost) +
                    "** | shipping: **" + cash(shipping) +
                    "** | proceeds: **" + cash(proceeds) + "**.";
            }
        } else if (operation == "fund") {
            int64_t amount = 0;
            if (!player.business_level || !parse_amount(1, amount) || !need_cash(amount)) {
                if (message.empty()) message = "Usage: `~business fund <amount>`.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                wallet.wallet_cents -= amount;
                player.business_cash_cents += amount;
                message = "Capitalized the business with **" + cash(amount) + "**.";
            }
        } else if (operation == "withdraw") {
            int64_t amount = 0;
            if (!parse_amount(1, amount) || player.business_cash_cents < amount) {
                if (message.empty()) message =
                    "Usage: `~business withdraw <amount>` within available business cash.";
                result = ECONOMY_INSUFFICIENT_FUNDS;
            } else {
                const int64_t tax = amount * government.tax_basis_points / 10000;
                player.business_cash_cents -= amount;
                wallet.wallet_cents += amount - tax;
                government.treasury_cents += tax;
                crime.taxes_paid_cents += tax;
                message = "Distributed **" + cash(amount - tax) +
                          "** after **" + cash(tax) + "** in business taxes.";
            }
        } else if (operation == "upgrade") {
            const int64_t cost = 200000LL * std::max<uint32_t>(1, player.business_level);
            if (!player.business_level || player.business_level >= 5 ||
                player.business_cash_cents < cost) {
                message = "Upgrade requires business cash of **" + cash(cost) +
                          "**; maximum level is 5.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                player.business_cash_cents -= cost;
                ++player.business_level;
                message = "Expanded to business level **" +
                          std::to_string(player.business_level) + "**.";
            }
        } else {
            message = "**" +
                std::string(kIndustries[std::min<uint32_t>(business.industry, 5)]) +
                " business — level " + std::to_string(player.business_level) +
                "**\nCash / debt: **" + cash(player.business_cash_cents) + " / " +
                cash(business.debt_cents) + "** | Reputation: **" +
                std::to_string(business.reputation) + "/100**\nRaw / finished: **" +
                std::to_string(business.raw_materials) + " / " +
                std::to_string(business.finished_goods) + "** | Equipment: **" +
                std::to_string(business.equipment_level) + "/5** | Marketing: **" +
                std::to_string(business.marketing) +
                "/100**\nLifetime revenue / profit: **" +
                cash(business.lifetime_revenue_cents) + " / " +
                cash(business.lifetime_profit_cents) +
                "**\n`/business input: start [industry]` "
                "`/business input: <fund|operate|withdraw|upgrade>`\n"
                "`/business input: export <server_id> <quantity>`";
        }
    } else if (action == "supply") {
        const uint32_t quantity = input.size() > 1 && input[0] == "buy"
            ? static_cast<uint32_t>(std::strtoul(input[1].c_str(), nullptr, 10)) : 0;
        const uint32_t industry = std::min<uint32_t>(business.industry, 5);
        int64_t unit_cost = industry
            ? kIndustryMaterialCost[industry] *
              std::max<int64_t>(5000, 10000 + guild.inflation_bp) / 10000 : 0;
        if (development.certifications & (1u << 2)) {
            unit_cost = unit_cost * 95 / 100;
        }
        const int64_t total = quantity && unit_cost <= kMaximumTransaction / quantity
            ? unit_cost * quantity : 0;
        if (!player.business_level || !quantity || quantity > 1000 ||
            business.raw_materials >
                std::numeric_limits<uint32_t>::max() - quantity ||
            player.business_cash_cents < total || !total) {
            message = "Usage: `~supply buy <1-1000>` using available company cash.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            player.business_cash_cents -= total;
            business.raw_materials += quantity;
            message = "Purchased **" + std::to_string(quantity) +
                "** raw units for **" + cash(total) + "** at **" +
                cash(unit_cost) + "** each.";
        }
    } else if (action == "produce") {
        const uint32_t quantity = !input.empty()
            ? static_cast<uint32_t>(std::strtoul(input[0].c_str(), nullptr, 10)) : 0;
        const uint32_t industry = std::min<uint32_t>(business.industry, 5);
        const uint32_t capacity = player.business_level * 10 +
                                  business.equipment_level * 20;
        int64_t unit_cost = industry ? kIndustryMaterialCost[industry] / 4 : 0;
        if (player.degree == 5) unit_cost = unit_cost * 85 / 100;
        const int64_t total = quantity && unit_cost <= kMaximumTransaction / quantity
            ? unit_cost * quantity : 0;
        if (!player.business_level || !quantity || quantity > capacity ||
            business.raw_materials < quantity ||
            business.finished_goods >
                std::numeric_limits<uint32_t>::max() - quantity ||
            player.business_cash_cents < total || !total) {
            message = "Production needs raw materials and company cash. Current batch "
                "capacity: **" + std::to_string(capacity) +
                "**. Usage: `~produce <quantity>`.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            business.raw_materials -= quantity;
            business.finished_goods += quantity;
            player.business_cash_cents -= total;
            business.reputation = std::min(100, business.reputation + 1);
            message = "Converted **" + std::to_string(quantity) +
                "** raw units into finished goods for **" + cash(total) + "**.";
        }
    } else if (action == "equipment") {
        const std::string operation = input.empty() ? "status" : input[0];
        int64_t cost = 150000LL * (business.equipment_level + 1);
        if (player.degree == 5) cost = cost * 85 / 100;
        if (development.certifications & (1u << 2)) cost = cost * 90 / 100;
        if (operation == "upgrade") {
            if (!player.business_level || business.equipment_level >= 5 ||
                player.business_cash_cents < cost) {
                message = "Equipment upgrade requires **" + cash(cost) +
                    "** in company cash; maximum level is 5.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                player.business_cash_cents -= cost;
                ++business.equipment_level;
                message = "Equipment upgraded to **" +
                    std::to_string(business.equipment_level) +
                    "/5**. Production capacity and efficiency improved.";
            }
        } else {
            message = "**Equipment:** " + std::to_string(business.equipment_level) +
                "/5 | Next upgrade: **" + cash(cost) +
                "**\nEngineering degrees reduce equipment and production costs.\n"
                "`~equipment upgrade`";
        }
    } else if (action == "marketing") {
        int64_t amount = 0;
        if (input.empty()) {
            message = "**Marketing momentum:** " +
                std::to_string(business.marketing) +
                "/100** (decays by 2 daily)\n`~marketing <amount>`";
        } else if (!player.business_level || !parse_amount(0, amount) ||
                   amount < 5000 || player.business_cash_cents < amount) {
            if (message.empty()) {
                message = "Marketing requires at least **" + cash(5000) +
                          "** in company cash.";
            }
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            player.business_cash_cents -= amount;
            const uint32_t gain = static_cast<uint32_t>(
                std::clamp<int64_t>(
                    amount / 5000 + (player.degree == 9 ? amount / 25000 : 0),
                    1, 30));
            business.marketing = std::min<uint32_t>(100, business.marketing + gain);
            business.reputation = std::min(100, business.reputation + int(gain / 5));
            message = "Campaign raised marketing momentum by **" +
                std::to_string(gain) + "** to **" +
                std::to_string(business.marketing) + "/100**.";
        }
    } else if (action == "businessloan") {
        const std::string operation = input.empty() ? "status" : input[0];
        int64_t amount = 0;
        const int64_t limit = player.business_level * 500000LL +
            business.equipment_level * 200000LL +
            std::max(0, business.reputation - 30) * 10000LL;
        if (operation == "borrow") {
            if (!parse_amount(1, amount) || !player.business_level ||
                player.credit_score < 600 || business.reputation < 35 ||
                business.debt_cents + amount > limit) {
                if (message.empty()) {
                    message = "Company loan denied. Requires **600** credit, **35** "
                        "reputation, and room under **" + cash(limit) + "**.";
                }
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                player.business_cash_cents += amount;
                business.debt_cents += amount + amount * 8 / 100;
                if (!business.next_payment_unix) {
                    business.next_payment_unix = now + 24 * 3600;
                }
                player.credit_score = std::max(300, player.credit_score - 5);
                message = "Company borrowed **" + cash(amount) +
                    "**; financed balance is **" + cash(business.debt_cents) +
                    "** with daily autopay.";
            }
        } else if (operation == "repay") {
            if (!parse_amount(1, amount) || !business.debt_cents ||
                player.business_cash_cents <= 0) {
                if (message.empty()) message = "Usage: `~businessloan repay <amount>`.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                amount = std::min({amount, business.debt_cents,
                                   player.business_cash_cents});
                player.business_cash_cents -= amount;
                business.debt_cents -= amount;
                if (!business.debt_cents) {
                    business.next_payment_unix = 0;
                    business.missed_payments = 0;
                }
                player.credit_score = std::min(850, player.credit_score + 3);
                message = "Company repaid **" + cash(amount) +
                    "**; remaining debt **" + cash(business.debt_cents) + "**.";
            }
        } else {
            message = "**Company financing**\nDebt / limit: **" +
                cash(business.debt_cents) + " / " + cash(limit) +
                "** | Missed: **" + std::to_string(business.missed_payments) +
                "** | Defaults: **" + std::to_string(business.defaults) +
                "**\n`~businessloan borrow <amount>` `~businessloan repay <amount>`";
        }
    } else if (action == "partnership") {
        const std::string operation = input.empty() ? "list" : input[0];
        if (operation == "offer") {
            const std::string partner =
                input.size() > 1 ? normalize_user(input[1]) : "";
            const uint32_t percent = input.size() > 2
                ? static_cast<uint32_t>(std::strtoul(input[2].c_str(), nullptr, 10)) : 0;
            int64_t contribution = 0;
            const bool already_exists = std::any_of(
                g_partnerships.begin(), g_partnerships.end(),
                [&](const BusinessPartnership& candidate) {
                    return candidate.guild_id == guild_id &&
                           candidate.owner_id == user_id;
                });
            if (!player.business_level || partner.empty() || partner == user_id ||
                percent < 5 || percent > 40 ||
                !parse_amount(3, contribution) || already_exists) {
                if (message.empty()) {
                    message = "Usage: `~partnership offer <user> <5-40%> <investment>`; "
                              "one partner per company.";
                }
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                g_partnerships.push_back({
                    g_next_partnership_id++, guild_id, user_id, partner,
                    percent * 100, contribution, false
                });
                message = "Offered <@" + partner + "> **" +
                    std::to_string(percent) + "%** for **" + cash(contribution) +
                    "** as partnership **#" +
                    std::to_string(g_partnerships.back().id) + "**.";
            }
        } else if (operation == "accept") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto offer = std::find_if(g_partnerships.begin(), g_partnerships.end(),
                [&](const BusinessPartnership& candidate) {
                    return candidate.id == id && candidate.guild_id == guild_id &&
                           candidate.partner_id == user_id && !candidate.accepted;
                });
            if (offer == g_partnerships.end() ||
                !need_cash(offer == g_partnerships.end()
                    ? 0 : offer->contribution_cents)) {
                if (offer == g_partnerships.end()) {
                    message = "That partnership offer is unavailable.";
                    result = ECONOMY_INVALID_ARGUMENT;
                }
            } else {
                const std::string owner_key = key_for(guild_id, offer->owner_id);
                if (!g_chaos_players[owner_key].business_level) {
                    message = "The underlying company no longer exists.";
                    result = ECONOMY_INVALID_ARGUMENT;
                } else {
                    wallet.wallet_cents -= offer->contribution_cents;
                    g_chaos_players[owner_key].business_cash_cents +=
                        offer->contribution_cents;
                    offer->accepted = true;
                    message = "Partnership **#" + std::to_string(id) +
                        "** activated. Future positive operations distribute your share.";
                }
            }
        } else if (operation == "dissolve") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto partnership = std::find_if(
                g_partnerships.begin(), g_partnerships.end(),
                [&](const BusinessPartnership& candidate) {
                    return candidate.id == id && candidate.guild_id == guild_id &&
                           candidate.owner_id == user_id && candidate.accepted;
                });
            if (partnership == g_partnerships.end() ||
                player.business_cash_cents < partnership->contribution_cents) {
                message = "The owner must fund the original contribution to dissolve "
                          "an active partnership.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                const int64_t buyout = partnership->contribution_cents;
                const std::string former_partner = partnership->partner_id;
                player.business_cash_cents -= buyout;
                ensure_player(guild_id, former_partner).wallet_cents += buyout;
                g_partnerships.erase(partnership);
                message = "Partnership dissolved for **" + cash(buyout) + "**.";
            }
        } else {
            std::ostringstream out;
            out << "**Business Partnerships**\n";
            size_t count = 0;
            for (const BusinessPartnership& partnership : g_partnerships) {
                if (partnership.guild_id != guild_id ||
                    (partnership.owner_id != user_id &&
                     partnership.partner_id != user_id)) continue;
                ++count;
                out << '#' << partnership.id << " owner <@" << partnership.owner_id
                    << "> / partner <@" << partnership.partner_id << "> — "
                    << partnership.share_basis_points / 100 << "% for "
                    << cash(partnership.contribution_cents) << " ["
                    << (partnership.accepted ? "ACTIVE" : "OFFERED") << "]\n";
            }
            if (!count) out << "No partnership offers or holdings.\n";
            out << "`~partnership offer <user> <percent> <investment>` "
                   "`~partnership accept <id>` `~partnership dissolve <id>`";
            message = out.str();
        }
    } else if (action == "payroll") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "hire") {
            const std::string employee = input.size() > 1 ? normalize_user(input[1]) : "";
            int64_t bonus = 0;
            if (employee.empty() || employee == user_id || !parse_amount(2, bonus) ||
                player.business_cash_cents < bonus || !player.business_level) {
                message = "Usage: `~payroll hire <@user|id> <signing-bonus>` with sufficient business cash.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                player.business_cash_cents -= bonus;
                ensure_player(guild_id, employee).wallet_cents += bonus;
                ++advanced.employees;
                message = "Hired <@" + employee + "> with **" + cash(bonus) +
                          "**. Employees: **" + std::to_string(advanced.employees) + "**.";
            }
        } else if (operation == "inventory") {
            uint32_t quantity = input.size() > 1
                ? static_cast<uint32_t>(std::strtoul(input[1].c_str(), nullptr, 10)) : 0;
            const int64_t cost = quantity * 5000LL;
            if (!quantity || quantity > 1000 ||
                business.raw_materials >
                    std::numeric_limits<uint32_t>::max() - quantity ||
                player.business_cash_cents < cost) {
                message = "Usage: `~payroll inventory <1-1000>`; units cost **" +
                          cash(5000) + "**.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                player.business_cash_cents -= cost;
                business.raw_materials += quantity;
                message = "Purchased **" + std::to_string(quantity) +
                          "** legacy supply units for **" + cash(cost) +
                          "**. `~supply buy` uses industry-sensitive pricing.";
            }
        } else {
            message = "**Employees:** " + std::to_string(advanced.employees) +
                " | **Raw materials:** " + std::to_string(business.raw_materials) +
                " | **Finished goods:** " + std::to_string(business.finished_goods) +
                "\n`~payroll hire <user> <bonus>` / `~payroll inventory <quantity>`";
        }
    } else if (action == "collectible") {
        const int64_t unit_price = collectible_market_price(guild_id, guild);
        const std::string operation = input.empty() ? "status" : input[0];
        uint32_t quantity = input.size() > 1
            ? static_cast<uint32_t>(std::strtoul(input[1].c_str(), nullptr, 10)) : 0;
        if (operation == "buy" && quantity && quantity <= 1000 &&
            need_cash(unit_price * quantity)) {
            wallet.wallet_cents -= unit_price * quantity;
            advanced.collectibles += quantity;
            for (uint32_t count = 0; count < quantity; ++count) {
                mint_collectible(guild_id, user_id, now);
            }
            message = "Bought **" + std::to_string(quantity) +
                      "** serialized launch relics for **" + cash(unit_price * quantity) + "**.";
        } else if (operation == "sell" && quantity && advanced.collectibles >= quantity) {
            const uint32_t sold_quantity = quantity;
            advanced.collectibles -= quantity;
            wallet.wallet_cents += unit_price * quantity;
            for (auto collectible = g_collectible_assets.begin();
                 collectible != g_collectible_assets.end() && quantity > 0;) {
                if (collectible->guild_id == guild_id &&
                    collectible->owner_id == user_id &&
                    collectible->auction_id == 0) {
                    collectible = g_collectible_assets.erase(collectible);
                    --quantity;
                } else {
                    ++collectible;
                }
            }
            message = "Sold relics for **" +
                      cash(unit_price * sold_quantity) + "**.";
        } else if (operation == "serials") {
            static constexpr std::array<const char*, 4> rarities = {
                "Common", "Uncommon", "Rare", "Mythic"
            };
            std::ostringstream out;
            out << "**Serialized Launch Relics**\n";
            size_t count = 0;
            for (const CollectibleAsset& collectible : g_collectible_assets) {
                if (collectible.owner_id != user_id ||
                    collectible.auction_id != 0 || count >= 12) continue;
                ++count;
                out << '#' << collectible.serial << " — "
                    << rarities[collectible.rarity] << " — "
                    << collectible.transfers << " transfer(s) — server `"
                    << collectible.guild_id << '`';
                if (!collectible.previous_owner_id.empty()) {
                    out << " — from <@" << collectible.previous_owner_id << '>';
                }
                out << '\n';
            }
            if (!count) out << "No unlisted serialized relics.\n";
            out << "Use `/collectible input: move <serial>` to carry one into "
                   "this economy, or `/auction` to transfer ownership.";
            message = out.str();
        } else if (operation == "move") {
            const uint64_t serial = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto collectible = std::find_if(
                g_collectible_assets.begin(), g_collectible_assets.end(),
                [&](const CollectibleAsset& candidate) {
                    return candidate.serial == serial &&
                           candidate.owner_id == user_id &&
                           candidate.auction_id == 0;
                });
            if (collectible == g_collectible_assets.end()) {
                message = "That globally owned, unlisted collectible was not found.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (collectible->guild_id == guild_id) {
                message = "That collectible is already present in this server economy.";
            } else {
                cross_advanced_key =
                    key_for(collectible->guild_id, user_id);
                AdvancedPlayer& origin =
                    g_advanced_players[cross_advanced_key];
                cross_advanced_before = origin;
                cross_advanced_changed = true;
                if (origin.collectibles) --origin.collectibles;
                ++advanced.collectibles;
                collectible->guild_id = guild_id;
                collectible->acquired_unix = now;
                message = "Moved global collectible **#" +
                    std::to_string(serial) +
                    "** into this server's local market.";
            }
        } else {
            message = "**Launch Relics — local / global:** " +
                std::to_string(advanced.collectibles) + " / " +
                std::to_string(global_collectible_count(user_id)) +
                " | Market price: **" + cash(unit_price) +
                "**\n`/collectible input: <buy|sell> <quantity>` · "
                "`/collectible input: serials|move <serial>`";
            if (operation == "buy" || operation == "sell") result = ECONOMY_INVALID_ARGUMENT;
        }
    } else if (action == "insurance") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "buy") {
            uint32_t level = input.size() > 1
                ? static_cast<uint32_t>(std::atoi(input[1].c_str())) : 0;
            static constexpr int64_t premiums[] = {0, 50000, 200000, 1000000};
            if (level < 1 || level > 3 || level <= advanced.insurance_level ||
                !need_cash(premiums[level])) {
                message = "Insurance tiers cost **" + cash(50000) + " / " +
                          cash(200000) + " / " + cash(1000000) +
                          "**. Buy a higher tier.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                wallet.wallet_cents -= premiums[level];
                advanced.insurance_level = level;
                message = "Insurance tier **" + std::to_string(level) + "** activated.";
            }
        } else if (operation == "claim") {
            if (!advanced.insurance_level ||
                (advanced.last_insurance_claim_unix &&
                 now - advanced.last_insurance_claim_unix < 24 * 3600)) {
                message = "No eligible claim. Policies require a 24-hour claim interval.";
                result = ECONOMY_COOLDOWN;
            } else {
                const int64_t insured_assets = portfolio_value(player, guild) +
                                               property_equity(guild_id, user_id);
                const int64_t coverage = std::min<int64_t>(
                    insured_assets / 20, advanced.insurance_level * 250000LL);
                advanced.last_insurance_claim_unix = now;
                wallet.wallet_cents += coverage;
                message = "Approved disaster claim: **" + cash(coverage) + "**.";
            }
        } else {
            message = "**Insurance tier:** " + std::to_string(advanced.insurance_level) +
                "\n`~insurance buy <1-3>` / `~insurance claim`";
        }
    } else if (action == "contract") {
        const std::string operation = input.empty() ? "list" : input[0];
        if (operation == "offer") {
            const std::string borrower = input.size() > 1 ? normalize_user(input[1]) : "";
            int64_t principal = 0;
            int64_t repayment = 0;
            const size_t open_contracts = static_cast<size_t>(std::count_if(
                g_contracts.begin(), g_contracts.end(),
                [&](const PlayerContract& contract) {
                    return contract.guild_id == guild_id && !contract.repaid &&
                           (contract.lender_id == user_id ||
                            contract.borrower_id == user_id);
                }));
            if (borrower.empty() || borrower == user_id || !parse_amount(2, principal) ||
                !parse_amount(3, repayment) || repayment < principal ||
                wallet.wallet_cents < principal || open_contracts >= 20) {
                message = "Usage: `~contract offer <@user|id> <principal> <repayment>`.";
                if (open_contracts >= 20) {
                    message = "Contract cap reached: settle existing agreements first.";
                }
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                g_contracts.push_back({g_next_contract_id++, guild_id, user_id, borrower,
                                       principal, repayment, false, false});
                message = "Offered loan contract **#" +
                          std::to_string(g_contracts.back().id) + "** to <@" + borrower +
                          ">. Funds transfer only after acceptance.";
            }
        } else if (operation == "accept") {
            const uint64_t id = input.size() > 1 ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto contract = std::find_if(g_contracts.begin(), g_contracts.end(),
                [&](const PlayerContract& item) {
                    return item.id == id && item.guild_id == guild_id &&
                           item.borrower_id == user_id && !item.accepted;
                });
            if (contract == g_contracts.end() ||
                g_players[key_for(guild_id, contract == g_contracts.end() ? user_id :
                                  contract->lender_id)].wallet_cents <
                    (contract == g_contracts.end() ? 0 : contract->principal_cents)) {
                message = "Contract unavailable or lender funds are insufficient.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                Player& lender = g_players[key_for(guild_id, contract->lender_id)];
                lender.wallet_cents -= contract->principal_cents;
                wallet.wallet_cents += contract->principal_cents;
                contract->accepted = true;
                message = "Accepted contract **#" + std::to_string(id) +
                          "** and received **" + cash(contract->principal_cents) + "**.";
            }
        } else if (operation == "pay") {
            const uint64_t id = input.size() > 1 ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto contract = std::find_if(g_contracts.begin(), g_contracts.end(),
                [&](const PlayerContract& item) {
                    return item.id == id && item.guild_id == guild_id &&
                           item.borrower_id == user_id && item.accepted && !item.repaid;
                });
            if (contract == g_contracts.end() || wallet.wallet_cents < contract->repayment_cents) {
                message = "Contract unavailable or repayment funds are insufficient.";
                result = ECONOMY_INSUFFICIENT_FUNDS;
            } else {
                wallet.wallet_cents -= contract->repayment_cents;
                g_players[key_for(guild_id, contract->lender_id)].wallet_cents +=
                    contract->repayment_cents;
                contract->repaid = true;
                player.credit_score = std::min(850, player.credit_score + 10);
                message = "Repaid contract **#" + std::to_string(id) + "** in full.";
            }
        } else {
            std::ostringstream out;
            out << "**Contracts**\n";
            size_t count = 0;
            for (const PlayerContract& contract : g_contracts) {
                if (contract.guild_id == guild_id &&
                    (contract.lender_id == user_id || contract.borrower_id == user_id)) {
                    ++count;
                    out << '#' << contract.id << " <@" << contract.lender_id << "> → <@"
                        << contract.borrower_id << "> " << cash(contract.principal_cents)
                        << " / repay " << cash(contract.repayment_cents) << " ["
                        << (contract.repaid ? "PAID" : contract.accepted ? "ACTIVE" : "OFFERED")
                        << "]\n";
                }
            }
            if (!count) out << "None.\n";
            out << "`~contract <offer|accept|pay|list> ...`";
            message = out.str();
        }
    } else if (action == "agreement") {
        const std::string operation = input.empty() ? "list" : input[0];
        if (operation == "offer") {
            const std::string type = input.size() > 1 ? input[1] : "";
            const std::string counterparty =
                input.size() > 2 ? normalize_user(input[2]) : "";
            const bool rental = type == "rental";
            const bool employment = type == "employment";
            size_t payment_index = rental ? 4 : 3;
            size_t days_index = rental ? 5 : 4;
            const uint64_t property_id = rental && input.size() > 3
                ? std::strtoull(input[3].c_str(), nullptr, 10) : 0;
            int64_t payment = 0;
            const uint32_t days = input.size() > days_index
                ? static_cast<uint32_t>(
                    std::strtoul(input[days_index].c_str(), nullptr, 10)) : 0;
            const size_t open = static_cast<size_t>(std::count_if(
                g_agreements.begin(), g_agreements.end(),
                [&](const ScheduledAgreement& agreement) {
                    return agreement.guild_id == guild_id && agreement.active &&
                           (agreement.issuer_id == user_id ||
                            agreement.counterparty_id == user_id);
                }));
            PropertyAsset* rental_property =
                rental ? find_property(guild_id, property_id) : nullptr;
            if ((!employment && !rental) || counterparty.empty() ||
                counterparty == user_id || !parse_amount(payment_index, payment) ||
                days < 1 || days > 30 || open >= 10 ||
                (employment && !player.business_level) ||
                (rental && (!rental_property ||
                            rental_property->owner_id != user_id))) {
                if (message.empty()) {
                    message = "Usage: `~agreement offer employment <user> <daily-pay> "
                              "<1-30 days>` or `~agreement offer rental <user> "
                              "<property-id> <daily-rent> <1-30 days>`.";
                }
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                g_agreements.push_back({
                    g_next_agreement_id++, guild_id, user_id, counterparty,
                    rental ? 1u : 0u, property_id, payment, days, 0, false, false
                });
                message = "Created standardized **" + type + " agreement #" +
                    std::to_string(g_agreements.back().id) + "** with <@" +
                    counterparty + ">: **" + cash(payment) + "** daily for **" +
                    std::to_string(days) + "** day(s). Acceptance is explicit.";
            }
        } else if (operation == "accept") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto agreement = std::find_if(
                g_agreements.begin(), g_agreements.end(),
                [&](const ScheduledAgreement& candidate) {
                    return candidate.id == id && candidate.guild_id == guild_id &&
                           candidate.counterparty_id == user_id &&
                           !candidate.accepted;
                });
            if (agreement == g_agreements.end()) {
                message = "That agreement is unavailable or belongs to someone else.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                agreement->accepted = agreement->active = true;
                agreement->next_payment_unix = now + 24 * 3600;
                message = "Agreement **#" + std::to_string(id) +
                          "** accepted; its first scheduled payment is due in 24 hours.";
            }
        } else if (operation == "cancel") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto agreement = std::find_if(
                g_agreements.begin(), g_agreements.end(),
                [&](const ScheduledAgreement& candidate) {
                    return candidate.id == id && candidate.guild_id == guild_id &&
                           candidate.issuer_id == user_id && !candidate.accepted;
                });
            if (agreement == g_agreements.end()) {
                message = "Only an unaccepted agreement can be cancelled unilaterally.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                g_agreements.erase(agreement);
                message = "Unaccepted agreement **#" + std::to_string(id) +
                          "** cancelled.";
            }
        } else {
            std::ostringstream out;
            out << "**Standardized Scheduled Agreements**\n";
            size_t count = 0;
            for (const ScheduledAgreement& agreement : g_agreements) {
                if (agreement.guild_id != guild_id ||
                    (agreement.issuer_id != user_id &&
                     agreement.counterparty_id != user_id)) continue;
                ++count;
                out << '#' << agreement.id << ' '
                    << (agreement.type == 0 ? "EMPLOYMENT" : "RENTAL")
                    << " <@" << agreement.issuer_id << "> ↔ <@"
                    << agreement.counterparty_id << "> — "
                    << cash(agreement.payment_cents) << "/day ×"
                    << agreement.remaining_payments << " ["
                    << (agreement.active ? "ACTIVE" :
                        agreement.accepted ? "COMPLETE/DEFAULTED" : "OFFERED")
                    << "]\n";
            }
            if (!count) out << "No agreements.\n";
            out << "`~agreement offer ...` `~agreement accept <id>` "
                   "`~agreement cancel <id>`";
            message = out.str();
        }
    } else if (action == "casino") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "start") {
            if (advanced.casino_level || !need_cash(500000)) {
                message = "A casino license and reserve require **" + cash(500000) + "**.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                wallet.wallet_cents -= 500000;
                advanced.casino_level = 1;
                advanced.casino_reserve_cents = 500000;
                message = "Casino opened with a **" + cash(500000) + "** reserve.";
            }
        } else if (operation == "operate") {
            if (!advanced.casino_level ||
                (advanced.last_casino_unix && now - advanced.last_casino_unix < 4 * 3600)) {
                message = "Casino unavailable or still on its four-hour customer cooldown.";
                result = ECONOMY_COOLDOWN;
            } else {
                std::uniform_int_distribution<int64_t> result_roll(-100000, 180000);
                const int64_t profit = result_roll(g_rng) * advanced.casino_level;
                advanced.casino_reserve_cents += profit;
                advanced.casino_profit_cents += profit;
                advanced.last_casino_unix = now;
                if (advanced.casino_reserve_cents < 0) {
                    advanced.casino_reserve_cents = 0;
                    advanced.casino_level = 0;
                    message = "The casino became insolvent and closed.";
                } else {
                    message = "Casino session result: **" + cash(profit) +
                              "**. Reserve: **" + cash(advanced.casino_reserve_cents) + "**.";
                }
            }
        } else if (operation == "withdraw") {
            int64_t amount = 0;
            if (!parse_amount(1, amount) || advanced.casino_reserve_cents < amount) {
                message = "Usage: `~casino withdraw <amount>` within the reserve.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                advanced.casino_reserve_cents -= amount;
                wallet.wallet_cents += amount;
                message = "Withdrew **" + cash(amount) + "** from casino reserves.";
            }
        } else {
            message = "**Player Casino level " + std::to_string(advanced.casino_level) +
                "** | Reserve: **" + cash(advanced.casino_reserve_cents) +
                "** | Lifetime result: **" + cash(advanced.casino_profit_cents) +
                "**\n`~casino <start|operate|withdraw>`";
        }
    } else if (action == "corporate") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "ipo") {
            if (corporate.public_company || player.business_level < 5 ||
                player.business_cash_cents < 1000000 ||
                business.reputation < 70 || business.equipment_level < 2) {
                message = "IPO requires a private level-5 company, **70** reputation, "
                          "equipment level **2**, and **" + cash(1000000) +
                          "** in company cash.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                const int64_t raised = 2500000 +
                    static_cast<int64_t>(business.marketing) * 10000 +
                    static_cast<int64_t>(business.reputation) * 5000;
                player.business_cash_cents -= 1000000;
                player.business_cash_cents += raised;
                corporate.public_company = true;
                guild.confidence = std::min(90, guild.confidence + 4);
                message = "IPO completed at a wildly optimistic valuation. **" +
                          cash(raised) + "** entered company cash; reputation and "
                          "marketing influenced demand.";
            }
        } else if (operation == "acquire") {
            const size_t index = input.size() > 1 ? stock_index(input[1]) : kStockCount;
            const int64_t cost = index < kStockCount ? guild.prices[index] * 1000 : 0;
            if (!corporate.public_company || index == kStockCount ||
                player.business_cash_cents < cost) {
                message = "Acquisition requires a public company and business cash equal to "
                          "1,000 target shares.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                player.business_cash_cents -= cost;
                ++corporate.acquisitions;
                guild.prices[index] = std::min<int64_t>(1000000000,
                    guild.prices[index] + guild.prices[index] / 10);
                message = "Acquired a controlling block of **" + std::string(kTickers[index]) +
                          "** for **" + cash(cost) + "**.";
            }
        } else {
            message = std::string("**Corporate status:** ") +
                (corporate.public_company ? "PUBLIC" : "PRIVATE") +
                " | Acquisitions: **" + std::to_string(corporate.acquisitions) +
                "**\n`~corporate ipo` / `~corporate acquire <ticker>`";
        }
    } else if (action == "stipend") {
        size_t role_count = 0;
        if (!input.empty() && !input[0].empty()) {
            role_count = 1 + static_cast<size_t>(
                std::count(input[0].begin(), input[0].end(), ','));
        }
        role_count = std::min<size_t>(role_count, 20);
        if (!role_count) {
            message = "No eligible Discord roles were present on this guild message.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else if (corporate.last_stipend_unix &&
                   now - corporate.last_stipend_unix < 24 * 3600) {
            message = "Role stipend already claimed in the last 24 hours.";
            result = ECONOMY_COOLDOWN;
        } else {
            const int64_t payout = static_cast<int64_t>(role_count) *
                                   settings.role_stipend_cents;
            wallet.wallet_cents += payout;
            corporate.last_stipend_unix = now;
            message = "Claimed **" + cash(payout) + "** from **" +
                      std::to_string(role_count) + "** eligible server roles.";
        }
    } else if (action == "gamble") {
        const bool explicit_game = !input.empty() &&
            (input[0] == "slots" || input[0] == "blackjack" ||
             input[0] == "roulette");
        const std::string game = explicit_game ? input[0] : "slots";
        int64_t amount = 0;
        if (!parse_amount(explicit_game ? 1 : 0, amount) ||
            amount > kMaximumTransaction / 10 ||
            !need_cash(amount)) {
            if (message.empty()) {
                message = "Usage: `~gamble [slots|blackjack|roulette] <amount>`";
            }
        } else {
            wallet.wallet_cents -= amount;
            player.gambling_wagered_cents += amount;
            std::uniform_int_distribution<int> roll(1, 1000);
            const int result_roll = roll(g_rng);
            int64_t payout = 0;
            if (game == "blackjack") {
                payout = result_roll <= 420 ? amount * 2 :
                         result_roll <= 500 ? amount : 0;
            } else if (game == "roulette") {
                payout = result_roll <= 470 ? amount * 2 : 0;
            } else {
                payout = result_roll <= 20 ? amount * 10 :
                         result_roll <= 180 ? amount * 2 : 0;
            }
            wallet.wallet_cents += payout;
            player.gambling_profit_cents += payout - amount;
            message = payout ? "**" + game + "** paid **" + cash(payout) +
                               "**. This will definitely happen forever."
                             : "**" + game + "** ate **" + cash(amount) +
                               "** and thanked you for supporting local employment.";
        }
    } else if (action == "crime") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "status") {
            message = "**Criminal Record**\nHeat: **" + std::to_string(crime.heat) +
                "/100** | Offenses: **" + std::to_string(crime.offenses) +
                "** | Successes: **" + std::to_string(crime.successes) +
                "**\nFines paid: **" + cash(crime.fines_paid_cents) +
                "** | Status: **" + (crime.jailed_until_unix > now
                    ? "JAILED for " + std::to_string(
                        (crime.jailed_until_unix - now + 59) / 60) + " minute(s)"
                    : "free, allegedly") +
                "**\n`~crime <pickpocket|fraud|heist>`";
        } else if (crime.jailed_until_unix > now) {
            message = "You cannot commit remote work crime from jail. Try `~bail`.";
            result = ECONOMY_COOLDOWN;
        } else if (crime.last_crime_unix && now - crime.last_crime_unix < 4 * 3600) {
            message = "Law enforcement is still reading your previous case file. Try again later.";
            result = ECONOMY_COOLDOWN;
        } else {
            int success_chance = 0;
            int64_t reward_low = 0;
            int64_t reward_high = 0;
            int64_t fine = 0;
            int64_t sentence = 0;
            uint32_t heat_gain = 0;
            if (operation == "pickpocket") {
                success_chance = 72;
                reward_low = 2500;
                reward_high = 7500;
                fine = 5000;
                sentence = 30 * 60;
                heat_gain = 10;
            } else if (operation == "fraud") {
                success_chance = 46 + (player.degree == 3 ? 8 : 0);
                reward_low = 15000;
                reward_high = 40000;
                fine = 30000;
                sentence = 2 * 3600;
                heat_gain = 25;
            } else if (operation == "heist") {
                success_chance = 24 + static_cast<int>(player.business_level) * 2;
                reward_low = 75000;
                reward_high = 200000;
                fine = 125000;
                sentence = 6 * 3600;
                heat_gain = 45;
            } else {
                message = "Usage: `~crime <pickpocket|fraud|heist>`";
                result = ECONOMY_INVALID_ARGUMENT;
            }
            if (result == ECONOMY_OK) {
                crime.last_crime_unix = now;
                crime.last_heat_decay_unix = now;
                ++crime.offenses;
                crime.heat = std::min<uint32_t>(100, crime.heat + heat_gain);
                const int roll = std::uniform_int_distribution<int>(1, 100)(g_rng);
                if (roll <= std::max(5, success_chance - static_cast<int>(crime.heat / 4))) {
                    const int64_t reward =
                        std::uniform_int_distribution<int64_t>(reward_low, reward_high)(g_rng);
                    wallet.wallet_cents += reward;
                    ++crime.successes;
                    guild.confidence = std::max(10, guild.confidence - 1);
                    message = "Crime succeeded. **" + cash(reward) +
                              "** appeared with absolutely no receipt. Heat is now **" +
                              std::to_string(crime.heat) + "**.";
                } else {
                    int64_t remaining_fine = fine;
                    const auto collect = [&](int64_t& account) {
                        const int64_t amount = std::min(account, remaining_fine);
                        account -= amount;
                        remaining_fine -= amount;
                        government.treasury_cents += amount;
                        crime.fines_paid_cents += amount;
                    };
                    collect(wallet.wallet_cents);
                    collect(wallet.checking_cents);
                    collect(player.savings_cents);
                    if (remaining_fine > 0) {
                        player.debt_cents += remaining_fine;
                        if (!lifecycle.next_payment_unix) {
                            lifecycle.next_payment_unix = now + 24 * 3600;
                        }
                    }
                    const int64_t law_discount = player.degree == 8 ? sentence / 4 : 0;
                    crime.jailed_until_unix = now + sentence - law_discount +
                        sentence * crime.heat / 100;
                    player.credit_score = std::max(300, player.credit_score -
                        static_cast<int>(15 + heat_gain / 2));
                    message = "Caught. Fine: **" + cash(fine) +
                              "**; unpaid fines became debt. Sentence: **" +
                              std::to_string((crime.jailed_until_unix - now + 59) / 60) +
                              " minute(s)**.";
                }
            }
        }
    } else if (action == "bail") {
        if (crime.jailed_until_unix <= now) {
            crime.jailed_until_unix = 0;
            message = "You are not currently incarcerated.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            const int64_t minutes = (crime.jailed_until_unix - now + 59) / 60;
            const int64_t cost = 10000 + static_cast<int64_t>(crime.heat) * 500 +
                                 minutes * 100;
            if (!need_cash(cost)) {
                message = "Bail is **" + cash(cost) + "**. Your wallet lacks legal optimism.";
            } else {
                wallet.wallet_cents -= cost;
                government.treasury_cents += cost;
                crime.fines_paid_cents += cost;
                crime.jailed_until_unix = 0;
                crime.heat /= 2;
                message = "Bail paid: **" + cash(cost) +
                          "**. You are free and only half as suspicious.";
            }
        }
    } else if (action == "government") {
        static constexpr const char* platforms[] = {
            "Low Tax", "Public Welfare", "Pro-Business"
        };
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "tariff" && input.size() == 2 &&
            government.mayor_id == user_id) {
            const int percent = std::atoi(input[1].c_str());
            if (percent < 0 || percent > 25) {
                message = "Tariffs must be between 0 and 25 percent.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                dynamics.tariff_basis_points = percent * 100;
                message = "Import tariff set to **" +
                    std::to_string(percent) +
                    "%**. Trading partners have begun drafting strongly worded statements.";
            }
        } else if (operation == "treaty" && input.size() == 2 &&
                   government.mayor_id == user_id &&
                   valid_id(input[1].c_str()) && input[1] != guild_id &&
                   g_guilds.find(input[1]) != g_guilds.end()) {
            dynamics.trade_partner = input[1];
            message = "Proposed a bilateral trade agreement with server `" +
                input[1] + "`. Tariff relief activates when both governments "
                "select one another.";
        } else if (operation != "status" && operation != "tariff" &&
                   operation != "treaty") {
            message = "Usage: `/government input: <tariff 0-25|treaty server_id>` "
                      "(current mayor only).";
            result = ECONOMY_INVALID_ARGUMENT;
        } else if ((operation == "tariff" || operation == "treaty") &&
                   government.mayor_id != user_id) {
            message = "Only the sitting mayor can change international trade policy.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            message = "**Guild Government**\nMayor: " +
                (government.mayor_id.empty() ? std::string("vacant")
                                             : "<@" + government.mayor_id + ">") +
                "\nPlatform: **" +
                platforms[std::min<uint32_t>(government.platform, 2)] +
                "** | Tax rate: **" +
                std::to_string(government.tax_basis_points / 100.0) +
                "%** | Import tariff: **" +
                std::to_string(dynamics.tariff_basis_points / 100.0) +
                "%**\nTreasury / public debt: **" +
                cash(government.treasury_cents) + " / " +
                cash(dynamics.government_debt_cents) +
                "**\nPolicy rate: **" +
                std::to_string(dynamics.policy_rate_bp / 100.0) +
                "%** | Trade partner: **" +
                (dynamics.trade_partner.empty() ? "none"
                                                : dynamics.trade_partner) +
                "**\n`/election` · `/taxes` · `/welfare` · "
                "`/government input: tariff|treaty ...`";
        }
    } else if (action == "taxes") {
        int64_t amount = 0;
        if (input.size() == 2 && input[0] == "pay" && parse_amount(1, amount) &&
            need_cash(amount)) {
            wallet.wallet_cents -= amount;
            government.treasury_cents += amount;
            crime.taxes_paid_cents += amount;
            player.credit_score = std::min(850, player.credit_score +
                static_cast<int>(std::min<int64_t>(10, amount / 10000)));
            crime.heat = crime.heat > 2 ? crime.heat - 2 : 0;
            message = "Paid **" + cash(amount) +
                      "** into the guild treasury. The bureaucracy noticed.";
        } else {
            message = "**Tax rate:** " +
                std::to_string(government.tax_basis_points / 100.0) +
                "%** | Treasury: **" + cash(government.treasury_cents) +
                "**\n`~taxes pay <amount>`";
        }
    } else if (action == "welfare") {
        const int64_t liquid = wallet.wallet_cents + wallet.checking_cents +
                               player.savings_cents + player.hysa_cents;
        if (player.job_tier != 0 || liquid >= 10000) {
            message = "Welfare requires unemployment and less than **" +
                      cash(10000) + "** in liquid assets.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else if (crime.last_welfare_unix &&
                   now - crime.last_welfare_unix < 24 * 3600) {
            message = "Welfare may be claimed once every 24 hours.";
            result = ECONOMY_COOLDOWN;
        } else if (government.treasury_cents < government.welfare_cents) {
            message = "The guild treasury cannot fund this payment. Civic austerity has arrived.";
            result = ECONOMY_INSUFFICIENT_FUNDS;
        } else {
            government.treasury_cents -= government.welfare_cents;
            wallet.wallet_cents += government.welfare_cents;
            crime.last_welfare_unix = now;
            message = "Received **" + cash(government.welfare_cents) +
                      "** in unemployment assistance.";
        }
    } else if (action == "election") {
        static constexpr const char* platforms[] = {
            "lowtax", "welfare", "business"
        };
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "run") {
            uint32_t platform = 3;
            if (input.size() > 1) {
                for (size_t i = 0; i < 3; ++i) {
                    if (input[1] == platforms[i]) platform = static_cast<uint32_t>(i);
                }
            }
            const bool already_running = std::any_of(
                g_candidates.begin(), g_candidates.end(),
                [&](const ElectionCandidate& candidate) {
                    return candidate.guild_id == guild_id &&
                           candidate.user_id == user_id;
                });
            if (platform > 2 || already_running) {
                message = "Usage: `~election run <lowtax|welfare|business>`; one candidacy per election.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (government.election_end_unix > 0 &&
                       government.election_end_unix <= now) {
                message = "Polls are closed and the background count is pending.";
                result = ECONOMY_COOLDOWN;
            } else if (government.election_end_unix <= 0 &&
                       government.term_end_unix > now) {
                message = "The current mayoral term has not ended.";
                result = ECONOMY_COOLDOWN;
            } else if (!need_cash(10000)) {
                message = "Candidacy filing costs **" + cash(10000) + "**.";
            } else {
                if (government.election_end_unix <= 0) {
                    government.election_end_unix = now + 24 * 3600;
                }
                wallet.wallet_cents -= 10000;
                government.treasury_cents += 10000;
                g_candidates.push_back({guild_id, user_id, platform});
                message = "Candidacy filed on the **" +
                    std::string(platform == 0 ? "Low Tax" :
                                platform == 1 ? "Public Welfare" : "Pro-Business") +
                    "** platform. Voting closes in 24 hours.";
            }
        } else if (operation == "vote") {
            const std::string candidate_id =
                input.size() > 1 ? normalize_user(input[1]) : "";
            const bool exists = std::any_of(g_candidates.begin(), g_candidates.end(),
                [&](const ElectionCandidate& candidate) {
                    return candidate.guild_id == guild_id &&
                           candidate.user_id == candidate_id;
                });
            if (government.election_end_unix <= now || !exists) {
                message = "That candidate is not in an active election.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                g_votes[key] = candidate_id;
                message = "Vote recorded for <@" + candidate_id +
                          ">. Changing your vote before polls close is allowed.";
            }
        } else {
            std::ostringstream out;
            out << "**Mayoral Election**\n";
            size_t count = 0;
            const std::string prefix = std::string(guild_id) + '\x1f';
            for (const ElectionCandidate& candidate : g_candidates) {
                if (candidate.guild_id != guild_id) continue;
                ++count;
                const size_t votes = static_cast<size_t>(std::count_if(
                    g_votes.begin(), g_votes.end(), [&](const auto& vote) {
                        return vote.first.compare(0, prefix.size(), prefix) == 0 &&
                               vote.second == candidate.user_id;
                    }));
                out << "<@" << candidate.user_id << "> — **"
                    << platforms[std::min<uint32_t>(candidate.platform, 2)]
                    << "** — " << votes << " vote(s)\n";
            }
            if (!count) out << "No active candidates.\n";
            if (government.election_end_unix > now) {
                out << "Polls close in **"
                    << (government.election_end_unix - now + 3599) / 3600 << "h**.\n";
            }
            out << "`~election run <lowtax|welfare|business>` / `~election vote <user>`";
            message = out.str();
        }
    } else if (action == "property") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "buy") {
            const uint32_t tier = input.size() > 1
                ? static_cast<uint32_t>(std::strtoul(input[1].c_str(), nullptr, 10)) : 0;
            const bool mortgage = input.size() > 2 && input[2] == "mortgage";
            const bool valid_financing = input.size() <= 2 ||
                input[2] == "cash" || input[2] == "mortgage";
            const size_t holdings = static_cast<size_t>(std::count_if(
                g_properties.begin(), g_properties.end(), [&](const PropertyAsset& asset) {
                    return asset.guild_id == guild_id && asset.owner_id == user_id;
                }));
            const int64_t price = tier >= 1 && tier <= 3
                ? property_market_value(guild, tier) : 0;
            const int64_t due = mortgage ? price / 5 : price;
            if (tier < 1 || tier > 3 || holdings >= 5 || !valid_financing) {
                message = "Choose tier **1-3**; each player may hold five properties.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (mortgage && player.credit_score < 620) {
                message = "A mortgage requires **620** credit and a 20% down payment.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (!need_cash(due)) {
                // need_cash supplies the player-facing rejection.
            } else {
                wallet.wallet_cents -= due;
                g_properties.push_back({
                    g_next_property_id++, guild_id, user_id, tier, price, price,
                    mortgage ? price - due : 0,
                    mortgage ? now + 24 * 3600 : 0, now, 100, 0, false
                });
                player.property_level = std::max(player.property_level, tier);
                message = "Purchased **" + std::string(kProperties[tier]) + " #" +
                    std::to_string(g_properties.back().id) + "** for **" + cash(price) +
                    "**" + (mortgage ? " with **" + cash(due) +
                    "** down. Daily mortgage autopay begins in 24 hours." : ".");
            }
        } else if (operation == "maintain") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            PropertyAsset* asset = find_property(guild_id, id);
            int64_t cost = asset
                ? kPropertyBaseValues[asset->tier] / 50 : 0;
            if (development.certifications & (1u << 4)) cost = cost * 90 / 100;
            if (!asset || asset->owner_id != user_id || !need_cash(cost)) {
                if (!asset || asset->owner_id != user_id) {
                    message = "Usage: `~property maintain <owned-id>`.";
                    result = ECONOMY_INVALID_ARGUMENT;
                }
            } else {
                wallet.wallet_cents -= cost;
                asset->condition = 100;
                asset->market_value_cents = property_market_value(guild, asset->tier);
                message = "Restored property **#" + std::to_string(id) +
                          "** to perfect condition for **" + cash(cost) + "**.";
            }
        } else if (operation == "sell") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto asset = std::find_if(g_properties.begin(), g_properties.end(),
                [&](const PropertyAsset& candidate) {
                    return candidate.guild_id == guild_id &&
                           candidate.owner_id == user_id && candidate.id == id;
                });
            if (asset == g_properties.end() || asset->listed) {
                message = "That property is unavailable. Cancel its auction before an NPC sale.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                const int64_t fee = asset->market_value_cents / 25;
                const int64_t proceeds = asset->market_value_cents -
                                         asset->mortgage_cents - fee;
                if (proceeds < 0) {
                    message = "The property is underwater; its mortgage exceeds sale proceeds.";
                    result = ECONOMY_INVALID_ARGUMENT;
                } else {
                    wallet.wallet_cents += proceeds;
                    government.treasury_cents += fee;
                    crime.taxes_paid_cents += fee;
                    const uint64_t sold_id = asset->id;
                    g_properties.erase(asset);
                    recalculate_property_level(guild_id, user_id);
                    message = "Sold property **#" + std::to_string(sold_id) +
                        "** for **" + cash(proceeds) +
                        "** after mortgage payoff and 4% transfer tax.";
                }
            }
        } else {
            std::ostringstream out;
            out << "**Dynamic Real Estate Market**\n";
            for (uint32_t tier = 1; tier <= 3; ++tier) {
                out << tier << ". " << kProperties[tier] << " — "
                    << cash(property_market_value(guild, tier)) << '\n';
            }
            size_t count = 0;
            out << "**Your properties**\n";
            for (const PropertyAsset& asset : g_properties) {
                if (asset.guild_id != guild_id || asset.owner_id != user_id) continue;
                ++count;
                out << '#' << asset.id << ' ' << kProperties[asset.tier] << " — value "
                    << cash(asset.market_value_cents) << ", mortgage "
                    << cash(asset.mortgage_cents) << ", condition " << asset.condition
                    << '%' << (asset.listed ? " [AUCTION]" : "") << '\n';
            }
            if (!count) out << "No property holdings.\n";
            out << "Equity: **" << cash(property_equity(guild_id, user_id))
                << "**\n`~property buy <1-3> [cash|mortgage]` "
                   "`~property maintain <id>` `~property sell <id>`";
            message = out.str();
        }
    } else if (action == "auction") {
        const std::string operation = input.empty() ? "status" : input[0];
        if (operation == "list") {
            const std::string kind = input.size() > 1 ? input[1] : "";
            const uint64_t asset_id = input.size() > 2
                ? std::strtoull(input[2].c_str(), nullptr, 10) : 0;
            int64_t reserve = 0;
            int64_t buyout = 0;
            const bool reserve_ok = input.size() > 3 && parse_cash(input[3], reserve);
            const bool buyout_ok = input.size() <= 4 || parse_cash(input[4], buyout);
            const int hours = input.size() > 5 ? std::atoi(input[5].c_str()) : 24;
            const size_t seller_listings = static_cast<size_t>(std::count_if(
                g_auctions.begin(), g_auctions.end(), [&](const AuctionListing& listing) {
                    return listing.guild_id == guild_id && listing.seller_id == user_id;
                }));
            if ((kind != "relic" && kind != "property") || !asset_id || !reserve_ok ||
                !buyout_ok || (buyout && buyout < reserve) || hours < 1 || hours > 72 ||
                seller_listings >= 10) {
                message = "Usage: `~auction list <relic|property> <quantity|id> "
                          "<reserve> [buyout] [1-72h]`.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (kind == "relic") {
                if (asset_id > 1000 || advanced.collectibles < asset_id) {
                    message = "You do not own that many unlisted relics.";
                    result = ECONOMY_INSUFFICIENT_FUNDS;
                } else {
                    advanced.collectibles -= static_cast<uint32_t>(asset_id);
                    const uint64_t auction_id = g_next_auction_id++;
                    uint32_t serialized = 0;
                    for (CollectibleAsset& collectible : g_collectible_assets) {
                        if (collectible.guild_id == guild_id &&
                            collectible.owner_id == user_id &&
                            collectible.auction_id == 0 &&
                            serialized < asset_id) {
                            collectible.auction_id = auction_id;
                            ++serialized;
                        }
                    }
                    g_auctions.push_back({auction_id, guild_id, user_id, 0, 0,
                        static_cast<uint32_t>(asset_id), reserve, buyout, 0, "",
                        now + hours * 3600LL});
                    message = "Relics entered escrow as auction **#" +
                        std::to_string(g_auctions.back().id) + "** for " +
                        std::to_string(hours) + " hour(s).";
                }
            } else {
                PropertyAsset* asset = find_property(guild_id, asset_id);
                if (!asset || asset->owner_id != user_id || asset->listed ||
                    reserve <= asset->mortgage_cents + reserve / 50) {
                    message = "Property listing rejected: verify ownership, listing status, "
                              "and a reserve above mortgage plus the 2% fee.";
                    result = ECONOMY_INVALID_ARGUMENT;
                } else {
                    asset->listed = true;
                    g_auctions.push_back({g_next_auction_id++, guild_id, user_id, 1,
                        asset_id, 1, reserve, buyout, 0, "", now + hours * 3600LL});
                    message = "Property **#" + std::to_string(asset_id) +
                        "** entered auction **#" + std::to_string(g_auctions.back().id) +
                        "** for " + std::to_string(hours) + " hour(s).";
                }
            }
        } else if (operation == "bid") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            int64_t amount = 0;
            auto listing = std::find_if(g_auctions.begin(), g_auctions.end(),
                [&](const AuctionListing& candidate) {
                    return candidate.guild_id == guild_id && candidate.id == id &&
                           candidate.end_unix > now;
                });
            if (listing == g_auctions.end()) {
                message = "That auction is missing or already closed.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (!parse_amount(2, amount) || listing->seller_id == user_id) {
                if (listing->seller_id == user_id) {
                    message = "Sellers cannot bid on their own chaos.";
                    result = ECONOMY_INVALID_ARGUMENT;
                }
            } else {
                const int64_t minimum = listing->highest_bid_cents > 0
                    ? listing->highest_bid_cents +
                      std::max<int64_t>(100, listing->highest_bid_cents / 20)
                    : listing->reserve_cents;
                const bool same_bidder = listing->highest_bidder_id == user_id;
                const int64_t additional = amount -
                    (same_bidder ? listing->highest_bid_cents : 0);
                if (amount < minimum || (listing->buyout_cents &&
                                         amount >= listing->buyout_cents) ||
                    additional <= 0 || !need_cash(additional)) {
                    if (amount < minimum) {
                        message = "Next valid bid is **" + cash(minimum) + "**.";
                        result = ECONOMY_INVALID_ARGUMENT;
                    } else if (listing->buyout_cents && amount >= listing->buyout_cents) {
                        message = "Use `~auction buyout " + std::to_string(id) + "`.";
                        result = ECONOMY_INVALID_ARGUMENT;
                    }
                } else {
                    wallet.wallet_cents -= additional;
                    if (!same_bidder && !listing->highest_bidder_id.empty()) {
                        ensure_player(guild_id, listing->highest_bidder_id).wallet_cents +=
                            listing->highest_bid_cents;
                    }
                    listing->highest_bidder_id = user_id;
                    listing->highest_bid_cents = amount;
                    message = "Bid **" + cash(amount) + "** escrowed on auction **#" +
                              std::to_string(id) + "**.";
                }
            }
        } else if (operation == "buyout") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto listing = std::find_if(g_auctions.begin(), g_auctions.end(),
                [&](const AuctionListing& candidate) {
                    return candidate.guild_id == guild_id && candidate.id == id &&
                           candidate.end_unix > now;
                });
            if (listing == g_auctions.end() || !listing->buyout_cents ||
                listing->seller_id == user_id) {
                message = "That auction has no available buyout.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                const bool same_bidder = listing->highest_bidder_id == user_id;
                const int64_t additional = listing->buyout_cents -
                    (same_bidder ? listing->highest_bid_cents : 0);
                if (!need_cash(additional)) {
                    // need_cash supplies the rejection.
                } else {
                    wallet.wallet_cents -= additional;
                    if (!same_bidder && !listing->highest_bidder_id.empty()) {
                        ensure_player(guild_id, listing->highest_bidder_id).wallet_cents +=
                            listing->highest_bid_cents;
                    }
                    listing->highest_bidder_id = user_id;
                    listing->highest_bid_cents = listing->buyout_cents;
                    const uint64_t settled_id = listing->id;
                    const int64_t settled_price = listing->highest_bid_cents;
                    settle_auction(*listing, now);
                    g_auctions.erase(listing);
                    message = "Bought out auction **#" + std::to_string(settled_id) +
                              "** for **" + cash(settled_price) + "**.";
                }
            }
        } else if (operation == "cancel") {
            const uint64_t id = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            auto listing = std::find_if(g_auctions.begin(), g_auctions.end(),
                [&](const AuctionListing& candidate) {
                    return candidate.guild_id == guild_id && candidate.id == id &&
                           candidate.seller_id == user_id;
                });
            if (listing == g_auctions.end() || !listing->highest_bidder_id.empty()) {
                message = "Only your auction with no bids can be cancelled.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                settle_auction(*listing, now);
                g_auctions.erase(listing);
                message = "Auction **#" + std::to_string(id) +
                          "** cancelled; escrow returned.";
            }
        } else {
            const uint64_t requested = input.size() > 1
                ? std::strtoull(input[1].c_str(), nullptr, 10) : 0;
            std::ostringstream out;
            out << "**Player Auction House**\n";
            size_t count = 0;
            for (const AuctionListing& listing : g_auctions) {
                if (listing.guild_id != guild_id ||
                    (requested && listing.id != requested) || count >= 12) continue;
                ++count;
                out << '#' << listing.id << ' '
                    << (listing.asset_type == 0
                        ? std::to_string(listing.quantity) + " relic(s)"
                        : "property #" + std::to_string(listing.asset_id))
                    << " — seller <@" << listing.seller_id << "> — "
                    << (listing.highest_bid_cents
                        ? "bid " + cash(listing.highest_bid_cents)
                        : "reserve " + cash(listing.reserve_cents));
                if (listing.buyout_cents) out << " — buyout " << cash(listing.buyout_cents);
                out << " — " << std::max<int64_t>(
                    0, (listing.end_unix - now + 3599) / 3600) << "h\n";
            }
            if (!count) out << "No matching live auctions.\n";
            out << "`~auction list ...` `~auction bid <id> <amount>` "
                   "`~auction buyout <id>` `~auction cancel <id>`";
            message = out.str();
        }
    } else if (action == "mystats" || action == "mychart") {
        std::vector<PlayerHistoryPoint> points;
        for (auto it = g_player_history.rbegin();
             it != g_player_history.rend() && points.size() < 24; ++it) {
            if (it->guild_id == guild_id && it->user_id == user_id) {
                points.push_back(*it);
            }
        }
        std::reverse(points.begin(), points.end());
        if (points.empty()) {
            message = "Personal history begins with the next hourly economy snapshot.";
            result = ECONOMY_INVALID_ARGUMENT;
        } else if (action == "mystats") {
            const PlayerHistoryPoint& latest = points.back();
            const PlayerHistoryPoint& oldest = points.front();
            const int64_t change = latest.net_worth_cents - oldest.net_worth_cents;
            message = "**Personal Economic History**\nNet worth: **" +
                cash(latest.net_worth_cents) + "** | " +
                std::to_string(points.size()) + "h change: **" +
                (change >= 0 ? "+" : "") + cash(change) +
                "**\nLiquid / invested / debt: **" +
                cash(latest.liquid_cents) + " / " +
                cash(latest.invested_cents) + " / " +
                cash(latest.debt_cents) +
                "**\n`~mychart <networth|cash|debt|invested>`";
        } else {
            const std::string metric = input.empty() ? "networth" : input[0];
            if (metric != "networth" && metric != "cash" &&
                metric != "debt" && metric != "invested") {
                message = "Usage: `~mychart <networth|cash|debt|invested>`";
                result = ECONOMY_INVALID_ARGUMENT;
            } else if (points.size() < 2) {
                message = "Two hourly personal snapshots are required.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                std::vector<int64_t> values;
                for (const PlayerHistoryPoint& point : points) {
                    values.push_back(metric == "cash" ? point.liquid_cents :
                        metric == "debt" ? point.debt_cents :
                        metric == "invested" ? point.invested_cents :
                        point.net_worth_cents);
                }
                const auto [minimum, maximum] =
                    std::minmax_element(values.begin(), values.end());
                static constexpr const char* bars[] = {
                    "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"
                };
                std::string graph;
                for (int64_t value : values) {
                    const size_t level = *maximum == *minimum ? 3 :
                        static_cast<size_t>(
                            static_cast<long double>(value - *minimum) * 7 /
                            (*maximum - *minimum));
                    graph += bars[std::min<size_t>(7, level)];
                }
                message = "**Your " + metric + " — " +
                    std::to_string(values.size()) + " hourly points**\n`" +
                    graph + "`\nLow / high / latest: **" + cash(*minimum) +
                    " / " + cash(*maximum) + " / " + cash(values.back()) + "**";
            }
        }
    } else if (action == "economystats") {
        const EconomySnapshot stats = calculate_snapshot(guild_id, now);
        const int32_t employment_rate = stats.active_players
            ? static_cast<int32_t>(stats.employed_players * 100 / stats.active_players)
            : 0;
        const int64_t market_index = std::accumulate(
            stats.stock_prices.begin(), stats.stock_prices.end(), int64_t{0}) /
            static_cast<int64_t>(kStockCount);
        message = "**Guild Economic Observatory**\nPlayers: **" +
            std::to_string(stats.active_players) + "** | Employment: **" +
            std::to_string(employment_rate) + "%**\nMoney supply: **" +
            cash(stats.money_supply_cents) + "**\nAverage / median net worth: **" +
            cash(stats.average_net_worth_cents) + " / " +
            cash(stats.median_net_worth_cents) + "**\nTotal debt: **" +
            cash(stats.total_debt_cents) + "** | Bank stability: **" +
            std::to_string(stats.bank_stability) + "/100**\nBusiness value: **" +
            cash(stats.business_value_cents) + "** | Invested value: **" +
            cash(stats.investment_value_cents) + "**\nItem supply: **" +
            std::to_string(stats.item_supply) + "** | Market index: **" +
            cash(market_index) + "**\nInflation: **" +
            std::to_string(guild.inflation_bp / 100.0) +
            "%** | Confidence: **" + std::to_string(guild.confidence) +
            "/100**\n`~chart <money|wealth|debt|market|ticker> [2-24]`";
    } else if (action == "chart") {
        const std::string metric = input.empty() ? "wealth" : input[0];
        const size_t stock = stock_index(metric);
        size_t point_limit = input.size() > 1
            ? static_cast<size_t>(std::strtoul(input[1].c_str(), nullptr, 10)) : 12;
        point_limit = std::clamp<size_t>(point_limit, 2, 24);
        std::vector<int64_t> values;
        for (auto it = g_snapshots.rbegin();
             it != g_snapshots.rend() && values.size() < point_limit; ++it) {
            if (it->guild_id != guild_id) continue;
            int64_t value = 0;
            if (stock < kStockCount) {
                value = it->stock_prices[stock];
            } else if (metric == "money") {
                value = it->money_supply_cents;
            } else if (metric == "wealth") {
                value = it->median_net_worth_cents;
            } else if (metric == "debt") {
                value = it->total_debt_cents;
            } else if (metric == "market") {
                value = std::accumulate(
                    it->stock_prices.begin(), it->stock_prices.end(), int64_t{0}) /
                    static_cast<int64_t>(kStockCount);
            } else {
                message = "Usage: `~chart <money|wealth|debt|market|ticker> [2-24]`";
                result = ECONOMY_INVALID_ARGUMENT;
                break;
            }
            values.push_back(value);
        }
        if (result == ECONOMY_OK) {
            if (values.size() < 2) {
                message = "Not enough hourly history yet. Two background snapshots are required.";
                result = ECONOMY_INVALID_ARGUMENT;
            } else {
                std::reverse(values.begin(), values.end());
                const auto [minimum, maximum] =
                    std::minmax_element(values.begin(), values.end());
                static constexpr const char* bars[] = {
                    "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"
                };
                std::string graph;
                for (int64_t value : values) {
                    const size_t level = *maximum == *minimum ? 3 :
                        static_cast<size_t>(
                            static_cast<long double>(value - *minimum) * 7 /
                            (*maximum - *minimum));
                    graph += bars[std::min<size_t>(7, level)];
                }
                const double change_percent = values.front() != 0
                    ? static_cast<double>(
                        static_cast<long double>(values.back() - values.front()) * 100 /
                        std::max<int64_t>(1, std::abs(values.front())))
                    : 0.0;
                const std::string label = stock < kStockCount
                    ? std::string(kTickers[stock]) : metric;
                message = "**" + label + " — " + std::to_string(values.size()) +
                    " hourly points**\n`" + graph + "`\nLow: **" + cash(*minimum) +
                    "** | High: **" + cash(*maximum) + "** | Latest: **" +
                    cash(values.back()) + "**\nChange: **" +
                    (change_percent >= 0 ? "+" : "") +
                    std::to_string(change_percent) + "%**";
            }
        }
    } else if (action == "rank") {
        const std::string category = input.empty() ? "networth" : input[0];
        const std::array<std::string, 8> categories = {
            "networth", "cash", "credit", "debt", "business",
            "education", "bankruptcies", "taxes"
        };
        if (std::find(categories.begin(), categories.end(), category) ==
            categories.end()) {
            message = "Usage: `~rank <networth|cash|credit|debt|business|education|bankruptcies|taxes>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            std::vector<std::pair<std::string, int64_t>> ranking;
            const std::string prefix = std::string(guild_id) + '\x1f';
            for (const auto& entry : g_players) {
                if (entry.first.compare(0, prefix.size(), prefix) != 0) continue;
                const std::string player_id = entry.first.substr(prefix.size());
                const Player& ranked_wallet = entry.second;
                const ChaosPlayer ranked_player = g_chaos_players.count(entry.first)
                    ? g_chaos_players.at(entry.first) : ChaosPlayer{};
                const AdvancedPlayer ranked_advanced = g_advanced_players.count(entry.first)
                    ? g_advanced_players.at(entry.first) : AdvancedPlayer{};
                int64_t value = 0;
                if (category == "networth") {
                    value = ranked_wallet.wallet_cents + ranked_wallet.checking_cents +
                        ranked_player.savings_cents + ranked_player.hysa_cents +
                        portfolio_value(ranked_player, guild) +
                        advanced_asset_value(ranked_advanced, guild) +
                        property_equity(guild_id, player_id) -
                        ranked_player.debt_cents +
                        business_tangible_value(guild_id, player_id);
                } else if (category == "cash") {
                    value = ranked_wallet.wallet_cents + ranked_wallet.checking_cents +
                            ranked_player.savings_cents + ranked_player.hysa_cents;
                } else if (category == "credit") {
                    value = ranked_player.credit_score;
                } else if (category == "debt") {
                    value = ranked_player.debt_cents + ranked_advanced.margin_debt_cents;
                    for (const PropertyAsset& property : g_properties) {
                        if (property.guild_id == guild_id &&
                            property.owner_id == player_id) {
                            value += property.mortgage_cents;
                        }
                    }
                    const auto business_it = g_business_profiles.find(entry.first);
                    if (business_it != g_business_profiles.end()) {
                        value += business_it->second.debt_cents;
                    }
                } else if (category == "business") {
                    value = ranked_player.business_cash_cents +
                            ranked_advanced.casino_reserve_cents +
                            business_tangible_value(guild_id, player_id);
                } else if (category == "education") {
                    value = ranked_player.degree;
                } else if (category == "bankruptcies") {
                    value = ranked_player.bankruptcies;
                } else {
                    auto crime_it = g_crime_players.find(entry.first);
                    value = crime_it == g_crime_players.end()
                        ? 0 : crime_it->second.taxes_paid_cents;
                }
                ranking.emplace_back(player_id, value);
            }
            std::sort(ranking.begin(), ranking.end(), [](const auto& left, const auto& right) {
                if (left.second != right.second) return left.second > right.second;
                return left.first < right.first;
            });
            std::ostringstream out;
            out << "**" << category << " leaderboard**\n";
            const bool money_metric = category == "networth" || category == "cash" ||
                                      category == "debt" || category == "business" ||
                                      category == "taxes";
            for (size_t i = 0; i < std::min<size_t>(10, ranking.size()); ++i) {
                out << (i + 1) << ". <@" << ranking[i].first << "> — **"
                    << (money_metric ? cash(ranking[i].second)
                                     : std::to_string(ranking[i].second))
                    << "**\n";
            }
            if (ranking.empty()) out << "No ranked players yet.";
            message = out.str();
        }
    } else if (action == "economyinfo") {
        const std::string phase =
            dynamics.recession_hours >= 12 ? "recession" :
            dynamics.recession_hours > 0 ? "contraction warning" :
            dynamics.recovery_hours > 0 ? "organic recovery" :
            dynamics.trend >= 8 ? "expansion" :
            dynamics.trend <= -8 ? "cooling" : "balanced";
        message = "**Living Server Economy**\nIdentity: **" +
            std::string(kEconomyPersonalities[
                std::min<uint32_t>(dynamics.personality, 3)]) +
            "** | Cycle: **" + phase + "**\nInflation: **" +
            std::to_string(guild.inflation_bp / 100.0) + "%**\nUnemployment: **" +
            std::to_string(guild.unemployment_bp / 100.0) +
            "%**\nConsumer confidence: **" + std::to_string(guild.confidence) +
            "/100** | Behavior trend: **" +
            (dynamics.trend >= 0 ? "+" : "") +
            std::to_string(dynamics.trend) +
            "**\nPolicy rate: **" +
            std::to_string(dynamics.policy_rate_bp / 100.0) +
            "%** | Currency index: **" +
            std::to_string(dynamics.currency_index) +
            "**\nCapital in / out: **" +
            cash(dynamics.capital_inflow_cents) + " / " +
            cash(dynamics.capital_outflow_cents) +
            "**\nExports / imports: **" +
            cash(dynamics.exports_cents) + " / " +
            cash(dynamics.imports_cents) +
            "**\nRecession / recovery hours: **" +
            std::to_string(dynamics.recession_hours) + " / " +
            std::to_string(dynamics.recovery_hours) +
            "**\nThese values emerge from saving, spending, trading, hiring, "
            "capital movement, policy, and company results.";
    } else if (action == "news") {
        const std::string scope = input.empty() ? "all" : input[0];
        if (scope != "all" && scope != "local" && scope != "global") {
            message = "Usage: `/news input: <all|local|global>`";
            result = ECONOMY_INVALID_ARGUMENT;
        } else {
            std::ostringstream out;
            out << "**The Daily Tail · Living Newswire**\n";
            size_t shown = 0;
            for (auto it = g_news.rbegin();
                 it != g_news.rend() && shown < 8; ++it) {
                const bool visible = it->global || it->origin_guild == guild_id;
                if (!visible ||
                    (scope == "local" && it->global) ||
                    (scope == "global" && !it->global)) continue;
                ++shown;
                out << (it->global ? "🌐" : "📍") << " **"
                    << kNewsRarity[std::min<uint32_t>(it->rarity, 3)]
                    << "** " << it->headline;
                if (it->stage == 0) out << " *[developing]*";
                out << '\n';
            }
            if (!shown) {
                out << "No matching events yet. The simulation is gathering "
                       "behavioral and corporate signals.\n";
            }
            out << "\nLocal confidence **" << guild.confidence
                << "** · currency index **" << dynamics.currency_index
                << "** · `/news input: local|global`";
            message = out.str();
        }
    } else {
        message = "Unknown economy action.";
        result = ECONOMY_INVALID_ARGUMENT;
    }

    if (result == ECONOMY_OK) {
        ++global.lifetime_actions;
        sync_global_player(user_id, player, development, now);

        const int64_t wallet_spent =
            std::max<int64_t>(0, wallet_before.wallet_cents - wallet.wallet_cents);
        const int64_t saved =
            std::max<int64_t>(0,
                (player.savings_cents + player.hysa_cents) -
                (player_before.savings_cents + player_before.hysa_cents));
        const bool investment_action =
            action == "stock" || action == "orders" ||
            action == "derivatives" || action == "invest" ||
            action == "bonds" || action == "property" ||
            action == "business" || action == "equipment" ||
            action == "marketing" || action == "partnership" ||
            action == "corporate" || action == "auction";
        const bool sale_action =
            (action == "stock" && !input.empty() && input[0] == "sell") ||
            (action == "short" && !input.empty() && input[0] == "open") ||
            (action == "orders" && input.size() > 1 && input[1] == "sell");
        if (action != "forex") {
            if (investment_action) {
                add_bounded(dynamics.investment_cents, wallet_spent);
            } else {
                add_bounded(dynamics.spending_cents, wallet_spent);
            }
        }
        add_bounded(dynamics.saving_cents, saved);
        if (sale_action) {
            add_bounded(dynamics.selling_cents,
                std::max<int64_t>(wallet_spent,
                    wallet.wallet_cents - wallet_before.wallet_cents));
        }
        if (advanced.employees > advanced_before.employees) {
            dynamics.hires +=
                advanced.employees - advanced_before.employees;
        } else if (advanced.employees < advanced_before.employees) {
            dynamics.layoffs +=
                advanced_before.employees - advanced.employees;
        }
        if (player_before.business_level && !player.business_level) {
            dynamics.layoffs += std::max<uint32_t>(
                1, advanced_before.employees);
        }
        if (action == "pay" || action == "stock" || action == "auction" ||
            action == "contract" || action == "partnership") {
            global.lifetime_trade_cents +=
                static_cast<uint64_t>(wallet_spent);
        }

        const int64_t approximate_worth =
            wallet.wallet_cents + wallet.checking_cents +
            player.savings_cents + player.hysa_cents +
            portfolio_value(player, guild) +
            advanced_asset_value(advanced, guild) -
            player.debt_cents;
        if (global.lifetime_actions >= 1) global.achievements |= 1ull << 0;
        if (player_server_count(user_id) >= 2) global.achievements |= 1ull << 1;
        if (global.education_mask) global.achievements |= 1ull << 2;
        if (global.licenses) global.achievements |= 1ull << 3;
        if (player.business_level) global.achievements |= 1ull << 4;
        if (global_collectible_count(user_id)) global.achievements |= 1ull << 5;
        if (global.lifetime_forex_cents) global.achievements |= 1ull << 6;
        if (approximate_worth >= 100000000) global.achievements |= 1ull << 7;
        global.reputation = std::clamp(
            50 + static_cast<int32_t>(
                std::min<uint64_t>(30, global.lifetime_actions / 25)) +
            static_cast<int32_t>(
                std::min<uint64_t>(10, global.lifetime_trade_cents / 10000000)) -
            static_cast<int32_t>(std::min<uint32_t>(20, crime.offenses)),
            0, 100);
    }

    if (action != "history") {
        std::string summary = message.substr(0, 180);
        std::replace(summary.begin(), summary.end(), '\n', ' ');
        g_audit.push_back({guild_id, user_id, now, action, summary});
        if (g_audit.size() > 2000) {
            g_audit.erase(g_audit.begin(), g_audit.begin() + (g_audit.size() - 2000));
        }
    }

    if (!save_locked()) {
        wallet = wallet_before;
        player = player_before;
        advanced = advanced_before;
        corporate = corporate_before;
        settings = settings_before;
        lifecycle = lifecycle_before;
        crime = crime_before;
        government = government_before;
        guild = guild_before;
        exchange = exchange_before;
        bank_network = bank_network_before;
        business = business_before;
        development = development_before;
        security = security_before;
        if (global_player_was_new) g_global_players.erase(user_id);
        else g_global_players[user_id] = global_before;
        g_dynamics[guild_id] = dynamics_before;
        g_news = news_before;
        g_next_news_id = next_news_before;
        if (cross_target_changed) {
            g_players[cross_target_key] = cross_target_before;
        }
        if (cross_target_dynamics_changed) {
            g_dynamics[cross_target_guild] =
                cross_target_dynamics_before;
        }
        if (cross_advanced_changed) {
            g_advanced_players[cross_advanced_key] =
                cross_advanced_before;
        }
        g_contracts = contracts_before;
        g_orders = orders_before;
        g_audit = audit_before;
        g_candidates = candidates_before;
        g_votes = votes_before;
        g_properties = properties_before;
        g_auctions = auctions_before;
        g_next_property_id = next_property_before;
        g_next_auction_id = next_auction_before;
        g_partnerships = partnerships_before;
        g_next_partnership_id = next_partnership_before;
        g_collectible_assets = collectibles_before;
        g_next_collectible_serial = next_collectible_before;
        g_agreements = agreements_before;
        g_next_agreement_id = next_agreement_before;
        set_output(output, capacity, "The economy vault failed to save; nothing was committed.");
        return ECONOMY_STORAGE_ERROR;
    }
    set_output(output, capacity, message);
    return result;
}

int api_init(void*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    load_locked();
    log_message("[ECONOMY] Persistent local economy initialized");
    return 0;
}

void api_shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_loaded && !save_locked()) {
        log_message("[ECONOMY] Failed to flush state during shutdown");
    }
}

int api_has_capability(uint32_t capability) {
    return capability == static_cast<uint32_t>(EXT_CAP_CUSTOM);
}

void* api_get_function(const char* name) {
    if (!name) return nullptr;
    const std::string function_name(name);
    if (function_name == "economy_api_version") return reinterpret_cast<void*>(economy_api_version_impl);
    if (function_name == "economy_get_player") return reinterpret_cast<void*>(economy_get_player_impl);
    if (function_name == "economy_claim_daily") return reinterpret_cast<void*>(economy_claim_daily_impl);
    if (function_name == "economy_work") return reinterpret_cast<void*>(economy_work_impl);
    if (function_name == "economy_move_money") return reinterpret_cast<void*>(economy_move_money_impl);
    if (function_name == "economy_transfer") return reinterpret_cast<void*>(economy_transfer_impl);
    if (function_name == "economy_buy_item") return reinterpret_cast<void*>(economy_buy_item_impl);
    if (function_name == "economy_leaderboard") return reinterpret_cast<void*>(economy_leaderboard_impl);
    if (function_name == "economy_flush") return reinterpret_cast<void*>(economy_flush_impl);
    if (function_name == "economy_game_action") return reinterpret_cast<void*>(economy_game_action_impl);
    if (function_name == "economy_tick_all") return reinterpret_cast<void*>(economy_tick_all_impl);
    if (function_name == "economy_get_currency") return reinterpret_cast<void*>(economy_get_currency_impl);
    return nullptr;
}

ExtensionInfo g_info = {
    "local_economy",
    "Persistent local economies connected by a global simulation",
    "2.0.0",
    "Routine Team",
    EXT_TYPE_CUSTOM,
    static_cast<uint32_t>(EXT_CAP_CUSTOM),
    EXTENSION_API_VERSION
};

ExtensionAPI g_api = {
    api_init,
    api_shutdown,
    api_has_capability,
    api_get_function
};

}  // namespace

#ifdef _WIN32
#define ROUTINE_EXPORT __declspec(dllexport)
#else
#define ROUTINE_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

ROUTINE_EXPORT ExtensionInfo* extension_get_info() {
    return &g_info;
}

ROUTINE_EXPORT int extension_init(const ExtensionKernelBridge* bridge) {
    g_kernel = bridge;
    return g_api.init ? g_api.init(nullptr) : 0;
}

ROUTINE_EXPORT void extension_shutdown() {
    if (g_api.shutdown) g_api.shutdown();
    g_kernel = nullptr;
}

ROUTINE_EXPORT const ExtensionAPI* extension_get_api() {
    return &g_api;
}

}
