# Pre-push safety

Run:

```bash
./release_audit.sh
git status --short
git diff --cached --check
```

Never publish:

- `config.json` or bot tokens;
- `data/` or server economy records;
- logs, `.env` files, private keys, or certificates;
- build directories, core dumps, or compiled plugins.

The repository ignore rules cover these paths, while `release_audit.sh` checks
the intended public file set for forbidden artifacts and likely secrets.

If a token was ever exposed, reset it immediately in the Discord Developer
Portal. History rewriting removes text from Git; it does not revoke the
credential.

See [SECURITY.md](SECURITY.md) for the security model and
[PUBLIC_RELEASE.md](PUBLIC_RELEASE.md) for creating the independent public
repository.
