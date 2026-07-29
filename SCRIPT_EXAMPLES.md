# Example Scripts

These are complete, working examples you can post directly in Discord.

## 1. Simple Greeter

```yaml
script: simple_greeter
on: message.create

when:
  starts_with: "!hi"

do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Hey {{author.username}}!"
```

## 2. Echo Bot

```yaml
script: echo
on: message.create

when:
  starts_with: "!echo"

do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "{{content}}"
```

## 3. Status Reporter

```yaml
script: status
on: message.create

when:
  contains: "!status"

do:
  - module: log
    args:
      message: "Status requested by {{author.username}}"
  
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Bot Status: ONLINE\n Engine: C++\n Modules: Loaded"
```

## 4. Help Command via Script

```yaml
script: help
on: message.create

when:
  starts_with: "!scripthelp"

do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "**Available Scripts:**\nUse !hi to greet\nUse !status for status\nUse !echo to echo"
```

## 5. Welcome Message (Server Join)

```yaml
script: welcome
on: guild.create

do:
  - module: log
    args:
      message: "Bot joined a new server!"
```

## 6. Regex Pattern Matcher

```yaml
script: detect_links
on: message.create

when:
  matches_regex: "https?://.*"

do:
  - module: log
    args:
      message: "Link detected from {{author.username}}: {{content}}"
```

## 7. Channel-Specific Response

```yaml
script: announcements
on: message.create

when:
  in_channel: "123456789012345678"
  contains: "!announce"

do:
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Announcement posted by {{author.username}}"
```

## 8. Multiple Actions

```yaml
script: complex_workflow
on: message.create

when:
  starts_with: "!process"

do:
  - module: log
    args:
      message: "Processing request from {{author.username}}"
  
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Processing your request..."
  
  - module: responder
    args:
      channel: "{{channel_id}}"
      content: "Done!"
```

## How to Load These

1. Copy the script you want
2. In Discord, type: `~script load`
3. Paste the script in a code block:
   ````
   ```yaml
   script: simple_greeter
   on: message.create
   
   when:
     starts_with: "!hi"
   
   do:
     - module: responder
       args:
         channel: "{{channel_id}}"
         content: "Hey {{author.username}}!"
   ```
   ````
4. Press Enter

The bot will validate and load it!

## Testing

After loading, test with:
- `!hi` for greeter
- `!status` for status
- `!echo test` for echo
- etc.

## Notes

- These examples only use built-in modules (`responder`, `log`)
- To invoke custom modules (C/C++/Lua/FORTRAN), just change the module name
- All scripts can be managed with `~script list`, `~script disable`, etc.

---

**The Stack:**
- C → Absolute control
- C++ → Structure without losing power  
- ASM → Raw trust issues
- Lua → Usability layer
- FORTRAN → Make the API uncomfortable
- **Scripts → Orchestration without chaos**
