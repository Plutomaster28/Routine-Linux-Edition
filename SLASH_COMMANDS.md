# Slash commands

Routine supports Discord chat-input application commands over the Gateway.
No public interaction web server or Interactions Endpoint URL is required.

## Lifecycle

1. Extensions and modules load.
2. Routine discovers kernel, native-module, and Lua commands.
3. The catalog is validated, deduplicated, and capped at Discord's 100
   chat-input-command limit.
4. Routine bulk-overwrites the configured guild or global command catalog.
5. Discord sends invocations as `INTERACTION_CREATE` Gateway events.
6. Routine immediately sends a deferred response.
7. The existing command callback runs.
8. Its first `send_message` edits the original interaction response; additional
   chunks become ordered follow-up messages.

This design preserves the existing module ABI. Native and Lua modules obtain
slash support without holding interaction tokens or implementing webhooks.

## Configuration

```json
"slash_commands": {
  "enabled": true,
  "register_on_start": true,
  "guild_id": ""
}
```

- `enabled`: enables interaction command registration.
- `register_on_start`: controls whether Routine owns and synchronizes the
  Discord catalog at startup.
- `guild_id`: registers instantly in one guild when populated; an empty value
  uses global registration.

The top-level `application_id` is optional. When omitted, the kernel uses the
bot user ID received in READY.

## Compatibility arguments

The current module ABI exposes command arguments as one string. Routine
therefore registers module commands with an optional `input` field and
passes that value to the unchanged callback. Core commands that take no
arguments omit the field.

The interaction parser already understands typed Discord values and renders
users, channels, and roles as mentions. A future module ABI can expose
per-command typed schemas without changing the interaction transport.

The bundled production catalog currently uses all 100 available command
slots. Tutorial modules live under `examples/` and are not loaded by default.

## Legacy commands

Prefix commands remain available for existing installations. They require
ordinary message events and Message Content Intent. Slash commands do not
depend on message content. The bundled configuration uses gateway intents
`37377` so both transports work; enable **Message Content Intent** in the
Discord Developer Portal. Slash-only deployments can use intent `1`.

## Operational notes

- Interaction callbacks must be acknowledged within three seconds. Routine
  uses a separate HTTP client for deferrals so ordinary outbound traffic does
  not sit ahead of the acknowledgement.
- Interaction tokens are never logged.
- Long responses use the same UTF-8-safe 2,000-byte chunker as channel
  messages.
- Bulk overwrite makes Routine's generated list authoritative for the selected
  guild or global scope.
