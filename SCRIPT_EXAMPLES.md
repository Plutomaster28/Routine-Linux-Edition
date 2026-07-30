# Script examples

These examples use only the two script actions implemented by Linux Edition:
`responder` and `log`. Load them with a legacy `~script load` message containing
the YAML in a fenced code block.

## Greeting

```yaml
script: greeting
on: message.create
when:
  starts_with: "hello routine"
do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Hello, {{author.username}}."
```

## Exact trigger

```yaml
script: exact_status
on: message.create
when:
  equals: "system status"
do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Routine is receiving message events."
```

## Channel-scoped notice

```yaml
script: channel_notice
on: message.create
when:
  in_channel: "YOUR_CHANNEL_ID"
  contains: "deployment"
do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Deployment discussion detected."
  - module: log
    args:
      message: "Deployment mention by {{author.id}}"
```

## Role-scoped response

```yaml
script: operator_ping
on: message.create
when:
  has_role: "YOUR_ROLE_ID"
  starts_with: "operator:"
do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Operator message acknowledged."
```

Role matching depends on the role data present in the event context. Validate
this behavior in a development server before relying on it.

## Regex trigger

```yaml
script: ticket_reference
on: message.create
when:
  matches_regex: "TICKET-[0-9]{4,8}"
do:
  - module: log
    args:
      message: "Ticket reference from {{author.username}}: {{content}}"
```

## Management

```text
/script input: list
/script input: show greeting
/script input: disable greeting
/script input: enable greeting
/script input: remove greeting
```

Scripts are currently in memory only and disappear when Routine restarts.
Unknown action modules do not invoke native modules. See
[SCRIPT_SYSTEM.md](SCRIPT_SYSTEM.md) for the exact boundaries.
