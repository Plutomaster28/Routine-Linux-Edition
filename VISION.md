# Routine Linux Edition — platform vision

Routine is a Discord kernel, not a monolithic collection of bot commands. The
kernel owns transport and lifecycle; modules own behavior; extensions own
specialized low-level capability. That boundary is what lets the Linux Edition
grow without turning every experiment into kernel debt.

## The three layers

```text
Discord Gateway and REST
          │
          ▼
┌──────────────────────────────┐
│ C++17 kernel                 │
│ transport · events · loading │
│ interactions · shutdown      │
└──────────────┬───────────────┘
               │ stable bridges
┌──────────────▼───────────────┐
│ Modules                      │
│ native C ABI · Lua           │
│ commands · game behavior     │
└──────────────┬───────────────┘
               │ named APIs
┌──────────────▼───────────────┐
│ Extensions                   │
│ storage · simulation · SIMD  │
│ reusable low-level systems   │
└──────────────────────────────┘
```

The YAML script engine sits beside modules as a deliberately constrained
message-automation layer. Today it can respond and log; it is not yet a
general native-module orchestrator.

## Kernel principles

The kernel should be:

- Linux-native and observable;
- boring under failure;
- strict about ABI boundaries;
- free of first-party game rules;
- safe around signals, threads, dynamic unloading, and Discord deadlines;
- modern in its Discord interface without abandoning compatible prefix users.

Kernel changes must benefit the framework. A game feature belongs in a module
or extension unless it exposes a genuinely reusable platform capability.

## Capability principles

Modules are specialists:

- Lua for fast, reviewable command behavior;
- C for a compact stable ABI;
- C++ for large native systems;
- Fortran for numerical routines;
- architecture-specific code only where measurement justifies it.

Extensions are beneath commands. They expose named functions through a stable
loader API and should not know how Discord messages are presented.

The economy demonstrates the intended split: its Discord menu is a module, its
authoritative simulation is an extension, and the kernel only sees generic
callbacks.

## Linux Edition priorities

1. Native Linux correctness before cross-platform claims.
2. Slash commands as the default user experience.
3. Legacy prefix compatibility as an explicit deployment mode.
4. Clean startup, reconnect, rate limiting, and shutdown.
5. Versioned ABIs and load-time rejection of incompatible binaries.
6. Public-repository hygiene with private configuration and data excluded.
7. Tests that exercise persistence and reload boundaries, not only pure
   utility functions.

Windows and macOS abstractions may remain in shared code, but this repository
only promises platforms exercised by its build and runtime verification.

## What Version 2 demonstrates

The economy expansion is a platform stress test:

- one global identity can coexist with isolated local ledgers;
- many server simulations can share persistent world events;
- cross-server transactions can remain atomic and bounded;
- player behavior can drive macro state without a scripted boom/recession
  switch;
- old database records can migrate forward without resets;
- a 100-command Discord ceiling can shape module UX without polluting the
  kernel.

The result should create stories from system interaction: capital leaves a
weak server, confidence falls, companies cut back, policy reacts, undervalued
assets attract buyers, and recovery emerges. The bot supplies rules and
feedback; players supply history.

## What remains intentionally unfinished

- Typed slash-command schemas in the module ABI
- Durable YAML-script persistence
- Validated native-module invocation from scripts
- Process isolation for untrusted native modules
- Native CI/runtime guarantees beyond Linux x86_64
- A general database backend for deployments that outgrow the compact economy
  store

Those are honest extension points, not features claimed by the current build.

## The one-liner

Routine is a small Linux Discord kernel designed to host systems far more
unhinged than the kernel itself.
