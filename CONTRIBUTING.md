# Contributing

Routine's current release target is native Linux and C++17.

## Development loop

```bash
./build_linux.sh
./release_audit.sh
```

Changes to the kernel should remain independent of the bundled economy.
Features needed by a module should normally become generic, documented kernel
capabilities rather than game-specific branches in `src/`.

## Expectations

- Keep the C module and extension boundaries C-compatible.
- Validate ABI tables and optional symbols before calling them.
- Do not detach threads that can outlive kernel-owned state.
- Preserve UTF-8 and Discord payload limits.
- Add focused tests for parsers, persistence, ABI behavior, and regressions.
- Avoid committing generated binaries, configuration, data, or logs.
- Update the public documentation when behavior or platform claims change.

Compiled modules are trusted in-process code. ABI changes must increment the
relevant version and document migration requirements.

Tutorial modules belong in `examples/modules/` so a default deployment does
not spend Discord's application-command quota on demonstrations.
