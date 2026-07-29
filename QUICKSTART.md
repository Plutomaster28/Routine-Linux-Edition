# Quick start

This guide targets a native Linux installation.

## 1. Create the Discord application

1. Open the [Discord Developer Portal](https://discord.com/developers/applications).
2. Create an application and add its bot user.
3. Copy the bot token. Treat it like a password.
4. Generate an installation URL with the `bot` and
   `applications.commands` scopes.
5. Give the bot only the server permissions its modules need.

Slash-only deployments do not require Message Content Intent. Enable that
privileged intent only if you want legacy `~` commands or modules that consume
ordinary message content, and configure the matching intent bits.

## 2. Install Linux dependencies

```bash
./install_deps_linux.sh
```

The script supports Debian/Ubuntu, Fedora-family systems, and Arch-family
systems. It installs the native compilers and libraries, creates a local
configuration when needed, and runs the complete build.

## 3. Configure Routine

```bash
cp config.example.json config.json
chmod 600 config.json
```

Edit only your local `config.json`:

```json
{
  "bot_token": "YOUR_BOT_TOKEN_HERE",
  "application_id": "",
  "guild_id": "",
  "slash_commands": {
    "enabled": true,
    "register_on_start": true,
    "guild_id": "YOUR_TEST_SERVER_ID"
  },
  "log_level": "info",
  "reconnect_attempts": 5,
  "heartbeat_interval": 41250,
  "gateway_intents": 1
}
```

- `application_id` is optional; Routine can learn it from READY.
- A slash-command `guild_id` makes command updates appear immediately in one
  test server. Leave it empty when you are ready for global registration.
- `gateway_intents: 1` is suitable for slash-command-first operation.
- Legacy prefix commands need the Guild Messages and Message Content intent
  bits and Message Content Intent enabled in the Developer Portal.

`config.json` is ignored by Git. Never copy the token into
`config.example.json`, source code, logs, issues, or commits.

## 4. Build and test

```bash
./build_linux.sh
```

The resulting executable is:

```text
build-linux/kernel/routine
```

## 5. Run

Run from any directory:

```bash
/path/to/Routine/build-linux/kernel/routine
```

Routine discovers the project root, loads extensions before modules, connects
to Discord, and synchronizes slash commands after the module catalog and
Discord identity are ready.

Try:

```text
/help
/ping
/list
/balance
/market
```

The old `~help` style remains available when message content events are
enabled.

## Command registration modes

During development, use a guild ID:

```json
"slash_commands": {
  "enabled": true,
  "register_on_start": true,
  "guild_id": "123456789012345678"
}
```

For a public bot, clear it:

```json
"guild_id": ""
```

Routine uses Discord's bulk-overwrite endpoint, so its generated catalog is
authoritative for the selected application scope.

## Troubleshooting

### Commands do not appear

- Confirm the bot was installed with `applications.commands`.
- Look for `Synchronized ... application command(s)` in the terminal.
- Use a guild registration while developing.
- Confirm the configured guild ID belongs to a server containing the bot.

### Discord closes with gateway code 4014

The configured intents request a privileged intent that is disabled in the
Developer Portal. For slash-only use, set `gateway_intents` to `1`. Otherwise,
enable the exact privileged intent requested.

### A module does not load

Run `./build_linux.sh` and inspect its ABI or missing-symbol error. Runtime
libraries must be the native Linux `.so` files produced by the current build.

### Economy data

Persistent data lives in `data/local_economy_v1.db`. Back up the entire
`data/` directory while the bot is stopped.
