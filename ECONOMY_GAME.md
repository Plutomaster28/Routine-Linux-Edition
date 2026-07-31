# Routine Local Economy Game

**Current release: 2.0.0**

This is the persistent full-chaos game built from
[`local_discord_economy_bot_plan.md`](local_discord_economy_bot_plan.md) and
expanded through [`economy-update.txt`](economy-update.txt).
It puts every major progression layer into one per-server simulation instead
of ending at the starter command loop.

## Architecture

- `lib/local_economy_extension.cpp` owns authoritative state, transaction
  validation, cooldowns, per-guild isolation, and persistence.
- `modules/local_economy_module.cpp` owns Discord commands, input parsing, and
  player-facing responses.
- `include/economy_extension_api.h` is the stable C ABI between them.
- Runtime state is written to `data/local_economy_v1.db`. The `data/`
  directory is ignored by Git. The version-3 save format retains compatibility
  with version-1 and version-2 saves, uses atomic replacement, and verifies a completion
  marker before loading. An incomplete primary automatically falls back to the
  last complete `.previous` snapshot.

Extensions load before modules, so the game module can resolve the economy API
during initialization. If the extension is absent or incompatible, the module
refuses to load instead of accepting unsafe or non-persistent transactions.

## Version 2 simulation

Version 1 established deep, isolated server economies. Version 2 connects
those economies without flattening them into one global balance:

- Player identity, education, professional licenses, achievements, reputation,
  serialized collectibles, and long-term statistics follow the Discord user.
- Every player-facing entry point, including the lightweight balance, claim,
  transfer, and shop commands, creates or updates that same global identity.
  Progression earned in one server is persisted before the command returns and
  is therefore visible immediately in another server.
- Wallets, banks, debt, businesses, jobs, property, governments, currencies,
  inflation, and markets remain local to each server.
- Foreign-exchange rates emerge from confidence, inflation, trade balance, and
  capital flow. Exchanges move value between the player's real local accounts
  and charge a spread.
- Spending, saving, investment, selling, hiring, layoffs, imports, exports, and
  capital flight feed each hourly macro cycle.
- Central-bank policy rates react to inflation and recessions. Prolonged
  contractions trigger treasury-backed stimulus or public debt issuance.
- Every listed company has a distinct personality. Expectations move before
  developing news resolves, so crowded rumors can succeed or unwind.
- Local news can progress into rare global events. Positive and negative events
  affect confidence, companies, and every connected economy at the appropriate
  scope.
- Server personalities—stable, high growth, financial hub, or chaotic—are
  classified from observed outcomes rather than chosen presets.
- Player companies can export production into another connected economy.
  Mayors control tariffs and can form reciprocal trade agreements.

The durable world layer is additive. Loading an existing Version 1 database
migrates it automatically and preserves all balances and assets.

## Beta data migration

The first Version 2 start against an older save performs one idempotent
best-of migration and writes `MV 1` into the database:

- education and professional-license masks are combined across every server;
- lifetime activity, reputation, and account age keep the strongest or oldest
  trustworthy value rather than adding duplicate server records together;
- global achievements are reconstructed from the migrated progression; and
- wallets, bank accounts, savings, debt, portfolios, businesses, property,
  items, governments, and markets stay unchanged and isolated per server.

Before that write, the extension creates
`data/local_economy_v1.db.pre_global_merge`. It never overwrites that archive,
so the untouched beta world remains available even after later `.previous`
snapshots rotate. If the archive cannot be created, migration pauses without
changing the loaded player data and retries on the next start.

To begin with an empty world instead, stop Routine and move
`data/local_economy_v1.db` plus its `.previous` file to an archive location
outside `data/`, then restart. Do not delete the files until the new world has
been verified. The whole `data/` directory remains excluded from Git.

## Commands

Slash commands are the primary interface. The same command names and input
strings work through the legacy `~` prefix when Message Content Intent is
enabled.

| Command | Description |
|---|---|
| `~economy` | Show the game command list |
| `/balance` | Show wallet, checking, net worth, and work count |
| `/profile` | Show one global player identity above the current server's local finances |
| `/profile input: global` | Show cross-server identity, education, licenses, achievements, and statistics |
| `/forex input: markets` | Browse connected currencies and emergent server identities |
| `/forex input: quote <server> [amount]` | Quote an exchange rate and spread |
| `/forex input: exchange <server> <amount>` | Move value between two existing local accounts |
| `~daily` | Claim 75–125 currency units once every 24 hours |
| `~work` | Earn an hourly wage based on career tier |
| `~deposit <amount>` | Move wallet cash into checking |
| `~withdraw <amount>` | Move checking funds into the wallet |
| `~pay <@user\|id> <amount>` | Transfer wallet cash within the server |
| `~shop` | Browse the basic item shop |
| `~buy <item> [quantity]` | Buy 1–100 basic items |
| `~inventory` | Show owned basic items |
| `~leaderboard` | Show the server’s top ten players by net worth |
| `~profile` | Show liquid funds, debt, credit, career, education, assets, and net worth |
| `~history` | Show persistent recent financial history |
| `~bank [select <1-6>]` | Compare banks, choose one, and inspect debt status |
| `~savings <deposit\|withdraw> <amount>` | Manage ordinary savings |
| `~hysa <deposit\|withdraw> <amount>` | Manage high-yield savings |
| `~cd <open amount\|close>` | Use a timed certificate of deposit |
| `~card [charge <amount>]` | Use a credit-limited cash-back card |
| `~loan <amount>` / `~repay <amount>` | Borrow against credit or repay debt |
| `~margin <borrow\|repay> <amount>` | Use credit-gated leveraged funds |
| `~bankruptcy` | Discharge debt while surrendering assets and destroying credit |
| `~career` / `~applyjob <tier>` | Build experience and unlock higher wages |
| `~college` / `~enroll <degree>` | Buy education and unlock professional careers |
| `~certifications [buy <type>]` | Earn prerequisite-gated professional credentials |
| `~skills` | Inspect derived financial, technical, management, negotiation, and risk skills |
| `~stipend` | Claim income derived from real Discord guild roles |
| `~market` | View the evolving nine-company local exchange |
| `~fundamentals <ticker>` | Inspect quotes, valuation, financials, volume, liquidity, and listing status |
| `~stock <buy\|sell> <ticker> <shares>` | Trade stocks with fees |
| `~orders <place\|cancel\|list> ...` | Manage escrowed persistent limit orders |
| `~short <open\|close> <ticker> <shares>` | Open or cover short liabilities |
| `~derivatives <call\|put> <ticker> <premium>` | Trade extreme-risk options |
| `~invest <asset> <buy\|sell> <amount>` | Use retirement, index, or commodity accounts |
| `~portfolio` | Value stocks, bonds, business cash, and property |
| `~bonds <buy\|sell> <amount>` | Use the low-risk investment path |
| `~business start [industry]` | Launch a food, manufacturing, technology, logistics, or retail company |
| `~business <fund\|operate\|withdraw\|upgrade>` | Capitalize, run, distribute from, or expand a company |
| `/business input: export <server> <quantity>` | Sell finished goods into a connected server economy |
| `~supply buy <quantity>` | Purchase macro- and industry-priced raw materials |
| `~produce <quantity>` | Spend company cash to convert inputs into finished goods |
| `~equipment [upgrade]` | Inspect or improve production capacity and efficiency |
| `~marketing [amount]` | Build customer-demand momentum that decays over time |
| `~businessloan <borrow\|repay> <amount>` | Use reputation- and credit-priced company financing |
| `~partnership ...` | Offer, accept, inspect, or dissolve player company equity |
| `~payroll <hire\|inventory> ...` | Hire players or use the legacy supply-purchase route |
| `~corporate <ipo\|acquire\|status>` | Take a company public or acquire listed firms |
| `~property` | Inspect live prices, owned property, mortgages, condition, and equity |
| `~property buy <tier> [cash\|mortgage]` | Buy one of three property classes with cash or 20% down |
| `~property maintain <id>` / `~property sell <id>` | Restore condition or sell after mortgage payoff and tax |
| `~auction [status\|<id>]` | Browse persistent player relic and property auctions |
| `~auction list <relic\|property> ...` | Place owned assets in escrow for 1–72 hours |
| `~auction bid <id> <amount>` / `~auction buyout <id>` | Bid with cash escrow or immediately settle a buyout |
| `~auction cancel <id>` | Cancel your listing before it receives a bid |
| `~collectible <buy\|sell> <quantity>` | Trade sentiment-priced serialized relics |
| `~collectible serials` | Inspect individual rarity, prior ownership, and transfer counts |
| `/collectible input: move <serial>` | Carry a globally owned relic into the current local market |
| `~gamble [slots\|blackjack\|roulette] <amount>` | Choose among distinct NPC casino odds |
| `~casino <start\|operate\|withdraw>` | Own a reserve-backed casino that can fail |
| `~insurance <buy\|claim> ...` | Cover assets and file cooldown-limited claims |
| `~contract <offer\|accept\|pay\|list> ...` | Use explicit player loan contracts |
| `~agreement <offer\|accept\|cancel\|list> ...` | Manage scheduled employment and rental agreements |
| `/news input: <all\|local\|global>` | Read persistent developing and resolved news |
| `/economyinfo` | Inspect cycle, identity, rates, currency strength, trade, and capital flow |
| `~econadmin ...` | Server-admin configuration for currency, starting funds, and role stipends |
| `~crime [pickpocket\|fraud\|heist]` | Take escalating criminal risks or inspect your record |
| `~bail` | Pay the guild treasury to end a jail sentence early |
| `/government` | Inspect the mayor, policy rate, tariffs, debt, welfare, treasury, and trade partner |
| `/government input: tariff <0-25>` | Let the sitting mayor set import tariffs |
| `/government input: treaty <server>` | Propose reciprocal tariff relief |
| `~election ...` | Run for mayor, vote, or inspect a live election |
| `~taxes [pay <amount>]` | Inspect policy or voluntarily fund the treasury |
| `~welfare` | Claim eligibility- and treasury-gated unemployment assistance |
| `~economystats` | Inspect measured guild money, wealth, debt, employment, and stability |
| `~chart <metric> [points]` | Render 2–24 hourly points for macro data or any stock |
| `~mystats` / `~mychart <metric>` | Inspect or chart personal net worth, cash, debt, and investments |
| `~rank <category>` | Rank net worth, cash, credit, debt, business, education, bankruptcies, or taxes |

Accounts are created on first interaction. The default starting balance is
50.00, but server administrators can change it for future players. Balances
use integer cents; floating-point currency is never used.

`~econadmin` is authorized by Discord guild ownership, Administrator, or
Manage Server permissions cached by the kernel. It cannot be unlocked through
command arguments:

```text
~econadmin status
~econadmin currency ¤ Chaos Bucks
~econadmin starting 250
~econadmin stipend 10
```

## Build and Test

Build the extension and module with the existing scripts:

```bash
./lib/build_extensions.sh
./modules/build_modules.sh
```

Then start the bot normally. Both shared libraries are discovered
automatically.

On Linux or WSL, the complete token-ready setup is:

```bash
./install_deps_linux.sh
```

That installs native Linux dependencies, builds and tests the kernel,
extensions, and modules, copies runtime libraries into place, and creates a
permission-restricted placeholder `config.json`.

The economy extension includes transaction and restart-persistence tests:

```bash
cmake -S lib -B /tmp/routine-economy-tests -G Ninja
cmake --build /tmp/routine-economy-tests \
  --target local_economy_extension economy_extension_test
ctest --test-dir /tmp/routine-economy-tests --output-on-failure
```

The command module has a separate ABI, formatting, and guild-routing test:

```bash
cmake -S modules -B /tmp/routine-economy-module-tests -G Ninja
cmake --build /tmp/routine-economy-module-tests \
  --target local_economy_module economy_module_test
ctest --test-dir /tmp/routine-economy-module-tests --output-on-failure
```

For an isolated development database, set `ROUTINE_ECONOMY_DATA` to a file
path before starting the bot.

## Implemented Systems

- A persistent global player layer for identity, education, licenses,
  reputation, achievements, global serialized collectibles, server count,
  lifetime actions, trade, and foreign-exchange statistics
- Persistent per-server behavioral flow accounts for consumption, saving,
  investment, asset sales, hiring, layoffs, capital movement, imports, and
  exports; hourly confidence, inflation, unemployment, and currency strength
  derive from those flows
- Real cross-server foreign exchange with strength-derived rates, spreads,
  value conservation between existing player accounts, and measurable capital
  flight/inflow
- International company exports, target-demand pricing, shipping costs,
  mayor-controlled tariffs, reciprocal trade agreements, and trade-balance
  feedback
- Persistent local and global news with common, uncommon, rare, and legendary
  events; developing expectations resolve into company-specific outcomes, and
  qualifying local successes propagate worldwide
- Company personalities for MEOW, Rat Mining, Yummy Burger, Intelligent,
  Enron, and the remaining exchange listings, with expectation pressure exposed
  through `/fundamentals`
- Emergent recession and recovery tracking, automatic policy-rate adjustment,
  treasury stimulus, emergency public debt issuance, and four behavior-derived
  server identities

- Persistent accounts, integer-cent transactions, transfers, items, shops,
  inventories, cooldowns, and leaderboards
- Six selectable banks with distinct APRs, approval scores, credit limits, and
  HYSA yields; scheduled minimum payments, automatic collection, delinquency,
  late fees, defaults, savings seizure, credit damage, and recoverable
  bankruptcy; each bank also has persistent stability, deposit-insured
  resolution, temporary account freezes, and uninsured-balance haircuts
- Experience-based careers, performance reviews, automatic promotions,
  recession-sensitive layoffs, retirement matching, degree-gated professional
  work, nine colleges, and nine degrees including Psychology
- Five professional certifications with real prerequisites and system effects,
  plus derived financial, technical, management, negotiation, and risk skills
- Nine plan-defined stocks with persistent revenue, expenses, profit, cash,
  debt, shares outstanding, fundamental value, market capitalization,
  sentiment, risk-sensitive spreads, rolling volume, and finite liquidity
- NPC market makers, bid/ask execution, player price pressure, market-order
  and escrowed limit-order partial fills, resting orders, and hourly depth
- Circuit-breaker halts, debt-driven dilution, two-for-one splits that adjust
  holdings/shorts/orders, distress, delisting, shareholder wipeout, order
  release, and 24-hour restructuring
- Trading fees, holdings, shorting, margin, calls, puts, bonds, portfolios,
  price charts, and detailed company-fundamental inspection
- CDs, cash-back cards, retirement accounts, index funds, commodities,
  dividends, margin interest, and early-withdrawal penalties
- Player businesses in five distinct industries with entry requirements,
  separate company cash, employees, payroll, raw materials, finished goods,
  industry-sensitive input/output prices, production capacity, customer demand,
  revenue, expenses, taxes, cooldowns, failure, and expansion
- Persistent equipment, decaying marketing momentum, reputation, lifetime
  revenue/profit, degree-sensitive operating efficiency, Engineering production
  discounts, macro-sensitive sales, and reputation/marketing-sensitive IPO demand
- Dedicated company loans with credit, reputation, equipment, and expansion
  limits; daily interest and autopay; delinquencies, late fees, credit damage,
  three-strike liquidation, and conversion of failed company debt into personal debt
- Persistent one-partner equity agreements with explicit offers, investment
  acceptance, company capitalization, automatic positive-profit distributions,
  owner-funded dissolution, bankruptcy treatment, and restart persistence
- Individually serialized collectibles with deterministic rarity, mint and
  acquisition times, prior owners, transfer counts, auction escrow, persistent
  provenance, and backward-compatible migration of existing relic stacks
- NPC gambling, player-owned reserve-backed casinos, insurance, persistent
  statistics, collectibles, and basic items
- Persistent identified property assets with macro-sensitive market values,
  rental income, condition decay, maintenance, 20%-down credit-gated mortgages,
  daily autopay, credit effects, missed payments, foreclosure, transfer taxes,
  NPC sales, and bankruptcy seizure
- A per-guild player auction house for relic stacks and property, including
  asset escrow, bid escrow, minimum raises, outbid refunds, optional buyouts,
  timed settlement, cancellation rules, ownership transfer, mortgage payoff,
  market fees, treasury funding, and restart persistence
- Validated player-loan contracts with explicit offer, borrower acceptance,
  repayment, and persistent status
- Persistent scheduled employment and property-rental agreements with explicit
  acceptance, daily settlement, finite terms, default consequences,
  cancellation, bankruptcy handling, and restart persistence
- Corporate IPOs, capital raising, target acquisitions, and market reactions
- Role-derived daily stipends using role IDs supplied by the v4 kernel bridge,
  not player-provided command arguments
- Persistent per-server currency name/symbol, future-player starting balance,
  and role-stipend configuration guarded by real Discord guild permissions
- Persistent criminal records with escalating risk, heat, cooldowns, fines,
  asset collection, unpaid-fine debt, credit damage, jail, bail, and automatic
  sentence/heat decay
- Player-run 24-hour mayoral elections with persistent candidates and mutable
  votes; automatic seven-day terms install low-tax, public-welfare, or
  pro-business policy
- A real per-guild treasury funded by candidacy fees, bail, fines, voluntary
  payments, and business-distribution taxes; treasury balance gates welfare
  rather than creating unlimited money
- Policy feedback into tax rates, welfare payments, unemployment, confidence,
  and player-business revenue
- Rolling seven-day hourly economic history for every guild, including money
  supply, average and median net worth, total debt, business and investment
  value, employment, item supply, bank stability, and all nine stock prices
- Rolling seven-day hourly histories for individual players, covering net worth,
  liquid funds, debt, and invested value with Discord-native personal charts
- Discord-native trend charts for money supply, median wealth, debt, the market
  index, and individual tickers
- Multi-category leaderboards for net worth, liquid cash, credit, debt,
  business value, education, bankruptcies, and actual taxes paid
- Per-guild inflation, unemployment, consumer confidence, market regimes,
  currency strength, capital flow, trade balance, policy rates, economic
  identities, and a persistent local/global Daily Tail newswire
- Cross-system effects: jobs improve income and credit; credit controls loans
  and derivatives; education gates careers and improves businesses; economic
  confidence affects markets and business revenue; bankruptcy seizes financial
  assets while preserving recovery income
- Automatic module ticks keep NPC markets, limit orders, macro events, rent,
  fundamentals, liquidity, halts, splits, dilution, delisting, mortgages,
  foreclosures, auctions, marketing decay, company-loan payments,
  delinquencies, and business liquidation alive
  without player commands; persistent history and `.previous` database
  snapshots provide auditability and manual rollback
- Release hardening includes global state-size ceilings, open-order, contract,
  and auction caps, transaction overflow checks, low-activity transfer ceilings,
  a 30-action-per-minute player rate limit, save-time invariants, completion-
  checked saves, atomic replacement, and automatic recovery from a truncated
  primary database

Unsafe free-form legal text remains intentionally excluded: contracts use
validated financial fields and explicit borrower acceptance. Every major
Version 2 system has an active, persistent gameplay loop. Future depth can add
shipping fleets, bilateral treaty ratification, resource-specific import
chains, and richer event content on top of the implemented trade and world
market foundation.

To restore the immediately previous database state, stop the bot, replace
`data/local_economy_v1.db` with `data/local_economy_v1.db.previous`, and
restart.
