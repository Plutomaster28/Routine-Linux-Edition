# Security policy

## Reporting a vulnerability

Do not open a public issue containing an exploit, bot token, interaction
token, private server data, or personally identifying data. Use the private
security-reporting channel configured on the eventual GitHub repository.

Until that channel exists, keep the report private and do not attach production
database files.

## Secrets and runtime data

The following are deliberately excluded from version control:

- `config.json`
- `data/`
- `logs/`
- `.env` files
- private keys and certificates
- compiled module and extension libraries
- build directories and core dumps

Run `./release_audit.sh` before the first public commit and before each
release. If a Discord bot token has ever entered a commit, terminal capture,
issue, or build log, reset it in the Discord Developer Portal before
publishing. Removing the text from the latest commit does not revoke it.

## Module trust model

Native modules and extensions execute in the Routine process with the same
operating-system permissions as the kernel. Load only code you trust. Lua
modules are an integration convenience, not a hardened security sandbox.

For public deployments:

- run Routine as a dedicated unprivileged user;
- grant the Discord bot only required permissions;
- keep `config.json` mode `0600`;
- back up private runtime data outside the repository;
- review native modules before loading them;
- keep system TLS and networking packages updated.
