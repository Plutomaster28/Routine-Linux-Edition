# Routine Linux Edition — implementation status

This document replaces the original Windows-era script-engine snapshot. It
describes the code that is currently built and tested in this repository.

## Release shape

Routine Linux Edition is a C++17 Discord kernel with three capability layers:

1. The kernel owns Discord Gateway/REST transport, interactions, rate limits,
   event dispatch, configuration, shutdown, and dynamic loading.
2. Modules own commands and Discord-facing behavior. Native modules use a
   versioned C ABI; Lua modules use the embedded Lua runtime.
3. Extensions expose lower-level capability APIs to native modules without
   adding game-specific behavior to the kernel.

The bundled economy uses a native module for commands and a native extension
for authoritative state. Removing those artifacts leaves a generic framework.

## Discord transport

- Gateway v10 over libwebsockets
- REST API v10 over libcurl
- Slash-command discovery and bulk synchronization after modules load
- Immediate interaction deferral on a dedicated HTTP client
- Original-response editing and ordered followups
- UTF-8-safe, lossless 2,000-character message chunking
- Optional legacy prefix transport through `MESSAGE_CREATE`
- Signal-safe Linux shutdown and deterministic reverse-order unloading

The default Gateway intents are `37377`, enabling slash commands plus guild and
direct-message prefix commands. Message Content Intent must also be enabled in
the Discord Developer Portal. Slash-only installations can use intent `1`.

## Runtime loading

- Linux `.so` modules and extensions are discovered at startup.
- Extensions initialize before modules.
- Native command tables are bounded and validated.
- Module API version 4 exposes message sending, logging, uptime, channel-to-
  guild resolution, extension lookup, cached roles, and guild-admin checks.
- Lua modules support command tables, raw message hooks, bot messaging, and
  reloads.
- Tutorial modules live in `examples/modules/` and are not loaded in
  production.

The generated production catalog is capped at Discord's 100 chat-input command
limit. The economy's old `bal` alias was replaced by `/forex` in Version 2 so
the catalog remains within that limit; `/balance` remains unchanged.

## Economy Version 2

The economy extension persists a backwards-compatible version-3 database:

- Existing beta databases receive a one-time best-of global progression
  migration. Credentials are combined, progression keeps the strongest
  values, and local money, debt, assets, and markets are never merged.
- The untouched input is preserved as
  `local_economy_v1.db.pre_global_merge`; an `MV 1` record makes the migration
  idempotent.

- Version 1 and Version 2 records load without reset.
- Saves use a temporary file, completion marker, atomic replacement, and a
  recoverable `.previous` snapshot.
- State invariants and collection ceilings are checked before persistence.
- A truncated primary database falls back to the previous complete snapshot.

Version 2 adds:

- global player identity with local accounts;
- cross-server currencies and foreign exchange;
- capital flight and international company exports;
- behavior-derived confidence, inflation, employment, and currency strength;
- endogenous recession and recovery tracking;
- automatic policy-rate and stimulus stabilizers;
- persistent local/global news progression and company expectations;
- distinct company personalities;
- mayor-controlled tariffs and reciprocal trade agreements;
- emergent stable, growth, financial-hub, and chaotic server identities.

See [ECONOMY_GAME.md](ECONOMY_GAME.md) for commands and mechanics.

## Script engine

The YAML script engine is intentionally narrower than the old documentation
claimed. It currently supports:

- `message.create` event dispatch;
- content, role, and channel conditions;
- `{{field.path}}` substitution;
- built-in `responder` and `log` actions;
- runtime load, list, show, enable, disable, and remove operations.

Arbitrary native-module invocation from scripts is not implemented. The code
marks that bridge as a future task, so documentation must not present it as
available. See [SCRIPT_SYSTEM.md](SCRIPT_SYSTEM.md).

## Verification

`./build_linux.sh` configures, compiles, and tests:

- the kernel and command/parser utilities;
- native extensions and database recovery;
- the economy module ABI and guild routing;
- native Fortran and C/C++ runtime artifacts.

The release audit separately checks ignore behavior, tracked secrets, runtime
artifacts, and public-source hygiene. CI repeats the Linux build on pushes and
pull requests.

## Known boundaries

- Linux x86_64 is the tested release target.
- Native modules execute in-process and must be trusted.
- Global slash-command updates can take time to propagate; use guild-scoped
  registration during development.
- The script engine is an automation layer, not a security sandbox for native
  code.
- The economy database is intentionally a compact custom format rather than a
  general-purpose relational database.
