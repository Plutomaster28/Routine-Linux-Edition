# Discord Bot Script System

**Chat writes intent. The system executes reality.**

## Philosophy

Scripts are **not code**. Scripts are **behavior descriptions**.

- Declarative: Describe what, not how
- Event-driven: React to Discord events
- Orchestration: Coordinate compiled modules
- Safe: No execution of untrusted code
- Auditable: Read the script, know what happens

## The Rule

> **Discord describes behavior. The filesystem holds capability.**

Scripts sent via Discord can **only** invoke modules that already exist as compiled DLLs/SOs. You cannot write new code through Discord—you can only wire together existing power.

This is what keeps the system safe and sane.

## Script Syntax

Scripts use a YAML-like syntax with three main sections:

```yaml
script: script_name
on: event_type

when:
  condition_type: value

do:
  - module: module_name
    args:
      key: value
```

### Required Fields

- **script**: Unique identifier for this script
- **on**: Event type to listen for (see Events section)
- **do**: List of actions to perform

### Optional Fields

- **when**: Conditions that must be met for the script to run

## Events

Available event types:

| Event | Description | Context Available |
|-------|-------------|-------------------|
| `message.create` | New message posted | `content`, `author`, `channel_id` |
| `ready` | Bot connected and ready | Bot info |
| `guild.create` | Bot joined a server | Guild info |

More events can be added by extending the engine.

## Conditions

The `when` clause filters when a script runs. All conditions must pass.

### Condition Types

| Type | Field | Description | Example |
|------|-------|-------------|---------|
| `starts_with` | `content` | Message starts with text | `starts_with: "!hello"` |
| `ends_with` | `content` | Message ends with text | `ends_with: "thanks"` |
| `contains` | `content` | Message contains text | `contains: "help"` |
| `equals` | `content` | Exact match | `equals: "ping"` |
| `matches_regex` | `content` | Regex pattern | `matches_regex: "^test.*"` |
| `has_role` | `author.roles` | User has role | `has_role: "Admin"` |
| `in_channel` | `channel_id` | Message in specific channel | `in_channel: "123456789"` |

## Actions

Actions invoke modules with arguments. Arguments can use template variables.

```yaml
do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Hello {{author.username}}!"
```

### Template Variables

Use `{{variable.path}}` to substitute context values:

| Variable | Description |
|----------|-------------|
| `{{content}}` | Message content |
| `{{author.id}}` | User ID |
| `{{author.username}}` | Username |
| `{{channel_id}}` | Channel ID |
| `{{guild_id}}` | Server ID |

### Built-in Modules

| Module | Purpose | Arguments |
|--------|---------|-----------|
| `responder` | Send a message | `channel`, `content` |
| `log` | Log to console | `message` |

Custom modules (C/C++/ASM/FORTRAN/Lua) can be invoked by name.

## Complete Examples

### 1. Auto-greeter

```yaml
script: greeter
on: message.create

when:
  starts_with: "!hello"

do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Hello {{author.username}}!"
```

### 2. Status Monitor

```yaml
script: status_check
on: message.create

when:
  contains: "!status"

do:
  - module: log
    args:
      message: "Status check requested by {{author.username}}"
      
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Bot is running!"
```

### 3. Module Orchestration

```yaml
script: analyze_sentiment
on: message.create

when:
  starts_with: "!analyze"

do:
  - module: parser
    args:
      text: "{{content}}"
      
  - module: lua_sentiment
    args:
      input: "{{parser.result}}"
      
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Sentiment: {{lua_sentiment.score}}"
```

## Using Scripts

### Loading a Script

Post in Discord with a code block:

```
~script load
```yaml
script: my_script
on: message.create

when:
  contains: "test"

do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Test received!"
```
```

The bot will parse, validate, and load the script.

### Managing Scripts

```bash
~script list                    # List all scripts
~script show my_script          # Show script details
~script disable my_script       # Temporarily disable
~script enable my_script        # Re-enable
~script remove my_script        # Delete permanently
```

## Permissions & Safety

### What Scripts CAN Do

React to Discord events  
Invoke pre-compiled modules  
Coordinate data flow between modules  
Use template variables for dynamic content  

### What Scripts CANNOT Do

Execute arbitrary code  
Access filesystem directly  
Make network requests (unless a module provides it)  
Invoke modules not already loaded  
Break out of the sandbox  

### The Safety Contract

1. **Scripts are declarative** - They describe, not compute
2. **Modules are compiled** - No runtime code generation
3. **Templates are substituted** - Not evaluated
4. **Validation is strict** - Bad scripts are rejected

## Architecture

```
Discord Message
      ↓
Script Engine (C++)
      ↓
Condition Check
      ↓
Action Execution
      ↓
Module Invocation (DLL/SO)
      ↓
Discord Response
```

The script engine is written in C++ for performance and safety. Scripts are parsed into an AST and validated before execution.

## Advanced: Context Passing

Scripts can store intermediate results:

```yaml
do:
  - module: analyzer
    args:
      data: "{{content}}"
    store_result_as: analysis
    
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Analysis: {{analysis.score}}"
```

## The Tech Stack (Reminder)

Your bot supports modules in:

- **C** - Absolute control
- **C++** - Structure with power
- **Assembly** - Raw machine code
- **Lua** - Scripting and logic
- **FORTRAN** - Scientific computing (because why not)

Scripts **orchestrate** these modules. They don't replace them.

## Best Practices

### DO

Keep scripts small and focused  
Use descriptive script names  
Comment complex logic (in Discord, before posting)  
Test conditions carefully  
Use built-in modules when possible  

### DON'T

Create mega-scripts that do everything  
Use overly broad conditions (e.g., `contains: "a"`)  
Invoke expensive operations on every message  
Store sensitive data in scripts  
Assume context variables always exist  

## Troubleshooting

### Script Won't Load

- Check YAML syntax (indentation matters!)
- Ensure `script`, `on`, and `do` fields are present
- Verify module names are correct

### Script Doesn't Run

- Check if it's enabled: `~script list`
- Verify conditions match your test case
- Check bot console for errors

### Template Variable Empty

- Ensure the context provides that variable
- Not all events have all variables
- Use `~script show` to debug

## Extending the System

### Adding New Events

Modify [script_engine.cpp](../src/script_engine.cpp) to dispatch new event types.

### Adding New Condition Types

Add to `ScriptCondition::Type` enum and implement in `evaluate()`.

### Creating Custom Modules

Write a module in C/C++/ASM/FORTRAN/Lua that implements the module interface. Scripts can then invoke it by name.

## Philosophy (Revisited)

This system exists because:

1. **Iteration speed matters** - Scripts can be written in Discord, no recompile
2. **Safety matters** - Scripts can't execute arbitrary code
3. **Power matters** - Compiled modules provide the real capabilities
4. **Clarity matters** - Reading a script tells you exactly what happens

The boundary between "script" and "code" is sacred:

- Scripts = behavior, orchestration, rules
- Code = capabilities, algorithms, power

Keep this distinction sharp, and the system stays brilliant instead of dangerous.

---

*"Discord is where behavior is described. The filesystem is where capability lives."*
