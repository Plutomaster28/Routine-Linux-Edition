#ifndef ECONOMY_EXTENSION_API_H
#define ECONOMY_EXTENSION_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define ECONOMY_EXTENSION_API_VERSION 1
#define ECONOMY_ITEM_COUNT 3
#define ECONOMY_MAX_LEADERBOARD 10

typedef enum {
    ECONOMY_OK = 0,
    ECONOMY_INVALID_ARGUMENT = 1,
    ECONOMY_COOLDOWN = 2,
    ECONOMY_INSUFFICIENT_FUNDS = 3,
    ECONOMY_PLAYER_NOT_FOUND = 4,
    ECONOMY_SELF_TRANSFER = 5,
    ECONOMY_STORAGE_ERROR = 6
} EconomyStatus;

typedef struct {
    int64_t wallet_cents;
    int64_t checking_cents;
    int64_t last_daily_unix;
    int64_t last_work_unix;
    uint64_t work_count;
    uint32_t item_quantities[ECONOMY_ITEM_COUNT];
} EconomyPlayerSnapshot;

typedef struct {
    char user_id[32];
    int64_t net_worth_cents;
} EconomyLeaderboardEntry;

typedef uint32_t (*economy_api_version_func)(void);
typedef int (*economy_get_player_func)(const char*, const char*, EconomyPlayerSnapshot*);
typedef int (*economy_claim_func)(const char*, const char*, int64_t, int64_t*, int64_t*);
typedef int (*economy_move_money_func)(const char*, const char*, int64_t, int);
typedef int (*economy_transfer_func)(const char*, const char*, const char*, int64_t);
typedef int (*economy_buy_item_func)(const char*, const char*, uint32_t, uint32_t, int64_t);
typedef size_t (*economy_leaderboard_func)(const char*, EconomyLeaderboardEntry*, size_t);
typedef int (*economy_flush_func)(void);
typedef int (*economy_game_action_func)(const char*, const char*, const char*,
                                        const char*, int64_t, char*, size_t);
typedef int (*economy_tick_all_func)(int64_t);
typedef int (*economy_get_currency_func)(const char*, char*, size_t, char*, size_t);

#ifdef __cplusplus
}
#endif

#endif
