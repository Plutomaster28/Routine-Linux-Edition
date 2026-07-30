# Script system

Routine includes a small YAML automation engine for message events. This guide
documents the implementation that exists today; it intentionally does not
repeat the original repository's unimplemented module-orchestration claims.

## Current capabilities

- Parse YAML supplied through a Discord message.
- React to `message.create`.
- Match content, channel, or cached role data.
- Substitute fields such as `{{channel_id}}` and `{{author.username}}`.
- Send a response through the built-in `responder` action.
- Write a console entry through the built-in `log` action.
- List, inspect, enable, disable, and remove loaded scripts.

Scripts are held in memory. `save_scripts` and `load_scripts` are not
implemented, so scripts do not survive a process restart.

Native module invocation from a script is also not implemented. An unknown
`module` value is currently logged and treated as a no-op. Use native or Lua
command modules when you need durable or privileged behavior.

## Loading a script

Script loading needs the full message code block, so use the legacy prefix
transport:

````text
~script load
```yaml
script: greeter
on: message.create
when:
  starts_with: "hello"
do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Hello, {{author.username}}."
```
````

Message Content Intent must be enabled. Slash interactions carry the `input`
string but do not contain a message code block, so `/script input: load` cannot
author a script.

Management operations work with the compatibility input string:

```text
/script input: list
/script input: show greeter
/script input: disable greeter
/script input: enable greeter
/script input: remove greeter
```

The equivalent legacy forms (`~script list`, and so on) also work.

## Schema

```yaml
script: unique_name
on: message.create
when:
  contains: "needle"
do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Matched {{content}}"
```

`script`, `on`, and `do` are required. `when` is optional. Every condition in
`when` must pass.

### Conditions

| YAML key | Meaning |
|---|---|
| `starts_with` | Message content starts with the value |
| `ends_with` | Message content ends with the value |
| `contains` | Message content contains the value |
| `equals` | Message content exactly equals the value |
| `matches_regex` | C++ regular-expression search |
| `has_role` | `author.roles` contains the role ID |
| `in_channel` | `channel_id` matches the value |

Unknown condition names currently become an always-true condition. Treat that
as a compatibility behavior, not input validation.

### Actions

`responder` requires `channel` and `content`:

```yaml
- module: responder
  args:
    channel: "{{channel_id}}"
    content: "A safe static or templated response"
```

`log` requires `message`:

```yaml
- module: log
  args:
    message: "Observed {{author.id}} in {{channel_id}}"
```

## Template data

The entire Discord `MESSAGE_CREATE` payload is copied into the script context.
Nested paths are resolved with dots:

- `{{content}}`
- `{{channel_id}}`
- `{{guild_id}}`
- `{{author.id}}`
- `{{author.username}}`

Missing values remain visible as their original template expression.

## Safety and operational limits

- Script YAML cannot execute shell commands or load native libraries.
- Regex evaluation catches invalid expressions, but complex expressions can
  still consume CPU. Do not accept scripts from untrusted users.
- The current script command has no built-in Discord permission gate.
  Restrict access operationally or add an authorization module before exposing
  script authoring in a public server.
- A responder can post as the bot in any channel ID supplied by the event
  context. Review scripts before loading them.
- Scripts execute synchronously on the event path.
- In-memory scripts vanish at restart.

## Honest roadmap

The source contains explicit placeholders for:

- durable script persistence;
- invoking a validated native module action;
- richer event coverage;
- permission controls for script management;
- schema validation for unknown conditions and actions.

Those are future kernel features, not current release functionality.
