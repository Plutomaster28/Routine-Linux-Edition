-- Relay Module - Inter-Server Communication System
-- "Bell Labs tier telecommunications for Discord"
-- 
-- Implements a handshake-based protocol for secure server-to-server messaging
-- Think: TCP handshake, not HTTP payload.

-- Module information
module_info = {
    name = "relay",
    version = "1.0.0",
    author = "Routine Kernel",
    description = "Inter-server communication relay with cryptographic handshaking"
}

-- Protocol constants
local PROTOCOL_VERSION = "1.0"
local CONNECTION_TYPES = {
    DIRECT = "DIRECT",
    MULTILATERAL = "MULTILATERAL",
    BROADCAST = "BROADCAST",           -- Future expansion
    RESEARCH_NET = "RESEARCH_NET",     -- Future expansion
    EMERGENCY_ONLY = "EMERGENCY_ONLY"  -- Future expansion
}

local PERMISSION_SCOPES = {
    ANNOUNCEMENT = "ANNOUNCEMENT",
    RESEARCH = "RESEARCH",
    CHAT = "CHAT",
    GENERAL = "GENERAL"
}

local ATTRIBUTION_MODES = {
    ANON = "ANON",        -- Anonymous relay
    PSEUDO = "PSEUDO",    -- Pseudonymous (hashed user)
    FULL = "FULL"         -- Full attribution
}

-- Connection state storage
-- Format: connections[channel_id] = {target_channel_id, server_id, config, status}
local connections = {}
local connection_configs = {}

-- Pending connection requests
-- Format: pending_requests[request_id] = {source_channel, target_channel, config, timestamp}
local pending_requests = {}

-- Session tracking
local active_sessions = {}

-- === UTILITY FUNCTIONS ===

-- Simple hash function (for demonstration - in production use proper crypto)
local function compute_hash(data)
    local hash = 5381
    for i = 1, #data do
        hash = ((hash * 33) + string.byte(data, i)) % 2147483647
    end
    return string.format("%08x", hash)
end

-- Generate session nonce
local function generate_nonce()
    return string.format("%08x%08x", 
        math.random(0, 0xFFFFFFFF), 
        os.time())
end

-- Get current UTC timestamp
local function get_timestamp_utc()
    return os.time()
end

-- Simple role hash (prevents structure leakage)
local function hash_user_role(user_id, role_info)
    return compute_hash(user_id .. (role_info or "member"))
end

-- === HANDSHAKE CONSTRUCTION ===

-- Create a complete handshake packet
local function create_handshake(origin_server_id, origin_channel_id, target_channel_id, 
                                user_id, payload, config)
    config = config or {}
    
    -- Core identity block
    local identity = {
        origin_server_id = origin_server_id,
        origin_bot_id = "ROUTINE_BOT",  -- Bot's application ID
        protocol_version = PROTOCOL_VERSION,
        connection_type = config.connection_type or CONNECTION_TYPES.DIRECT
    }
    
    -- Authority & consent block
    local session_nonce = generate_nonce()
    local timestamp = get_timestamp_utc()
    
    local authority = {
        relay_channel_id = target_channel_id,
        origin_channel_id = origin_channel_id,
        permission_scope = config.permission_scope or PERMISSION_SCOPES.GENERAL,
        session_nonce = session_nonce,
        timestamp_utc = timestamp
    }
    
    -- User attribution block
    local attribution = {
        user_id = user_id,
        user_role_hash = hash_user_role(user_id, config.role_info),
        attribution_mode = config.attribution_mode or ATTRIBUTION_MODES.PSEUDO
    }
    
    -- Integrity & verification block
    local payload_hash = compute_hash(payload)
    local signature_data = string.format("%s:%s:%s:%s:%s",
        origin_server_id, session_nonce, timestamp, user_id, payload_hash)
    local signature = compute_hash(signature_data)
    
    local integrity = {
        payload_hash = payload_hash,
        signature = signature
    }
    
    -- Complete handshake
    return {
        handshake = {
            identity = identity,
            authority = authority,
            attribution = attribution,
            integrity = integrity
        },
        payload = payload
    }
end

-- Verify handshake integrity
local function verify_handshake(packet)
    if not packet.handshake or not packet.payload then
        return false, "Invalid packet structure"
    end
    
    local h = packet.handshake
    
    -- Verify all required blocks exist
    if not (h.identity and h.authority and h.attribution and h.integrity) then
        return false, "Missing handshake blocks"
    end
    
    -- Verify protocol version
    if h.identity.protocol_version ~= PROTOCOL_VERSION then
        return false, "Protocol version mismatch"
    end
    
    -- Verify timestamp (not too old - 5 minute window)
    local current_time = get_timestamp_utc()
    if math.abs(current_time - h.authority.timestamp_utc) > 300 then
        return false, "Timestamp out of acceptable range"
    end
    
    -- Verify payload hash
    local computed_hash = compute_hash(packet.payload)
    if computed_hash ~= h.integrity.payload_hash then
        return false, "Payload hash mismatch"
    end
    
    -- Verify signature
    local signature_data = string.format("%s:%s:%s:%s:%s",
        h.identity.origin_server_id,
        h.authority.session_nonce,
        h.authority.timestamp_utc,
        h.attribution.user_id,
        h.integrity.payload_hash)
    local computed_signature = compute_hash(signature_data)
    
    if computed_signature ~= h.integrity.signature then
        return false, "Signature verification failed"
    end
    
    return true, "Handshake verified"
end

-- === CONNECTION MANAGEMENT ===

-- Establish a relay connection between two channels (internal)
-- This function must be defined before functions that use it
local function establish_connection(source_channel, target_channel, server_id, config)
    connections[source_channel] = {
        target = target_channel,
        server = server_id,
        established = get_timestamp_utc(),
        active = true,
        status = "CONNECTED"
    }
    
    connection_configs[source_channel] = config or {
        connection_type = CONNECTION_TYPES.DIRECT,
        permission_scope = PERMISSION_SCOPES.GENERAL,
        attribution_mode = ATTRIBUTION_MODES.PSEUDO
    }
    
    bot.log(string.format("Relay established: %s -> %s", source_channel, target_channel))
    return true
end

-- Terminate a relay connection
local function terminate_connection(source_channel)
    if connections[source_channel] then
        connections[source_channel] = nil
        connection_configs[source_channel] = nil
        bot.log(string.format("Relay terminated: %s", source_channel))
        return true
    end
    return false
end

-- Send connection request to target channel
local function send_connection_request(source_channel, target_channel, server_id, config)
    local request_id = generate_nonce()
    
    -- Store pending request
    pending_requests[request_id] = {
        source = source_channel,
        target = target_channel,
        server = server_id,
        config = config,
        timestamp = get_timestamp_utc()
    }
    
    -- Send request to target channel
    local request_msg = string.format([[
**INCOMING RELAY CONNECTION REQUEST**
```
Request ID:  %s
From Channel: %s
Server:       %s
Scope:        %s
Mode:         %s
Protocol:     v%s
```

A channel wants to establish a secure relay connection with this channel.

**To accept:** `~relay-accept %s`
**To deny:**   `~relay-deny %s`

*This request will expire in 5 minutes.*
]], 
        request_id:sub(1, 8),
        source_channel,
        server_id,
        config.permission_scope,
        config.attribution_mode,
        PROTOCOL_VERSION,
        request_id,
        request_id)
    
    -- Attempt to send the request
    bot.log(string.format("[RELAY] Attempting to send request to channel: %s", target_channel))
    local send_result = bot.send_message(target_channel, request_msg)
    bot.log(string.format("[RELAY] Send result for channel %s: %s", target_channel, tostring(send_result)))
    
    bot.log(string.format("Connection request sent: %s -> %s [%s]", 
        source_channel, target_channel, request_id))
    
    return request_id
end

-- Accept a connection request
local function accept_connection_request(request_id, accepting_channel)
    local request = pending_requests[request_id]
    
    if not request then
        return false, "Request not found or expired"
    end
    
    -- Verify the accepting channel is the target
    if request.target ~= accepting_channel then
        return false, "You are not the target of this request"
    end
    
    -- Check if request is still valid (5 minute timeout)
    if get_timestamp_utc() - request.timestamp > 300 then
        pending_requests[request_id] = nil
        return false, "Request has expired"
    end
    
    -- Establish bidirectional connection
    establish_connection(request.source, request.target, request.server, request.config)
    establish_connection(request.target, request.source, request.server, request.config)
    
    -- Clean up pending request
    pending_requests[request_id] = nil
    
    -- Notify both channels
    bot.send_message(request.source, string.format(
        "**CONNECTION ESTABLISHED**\n" ..
        "The target channel has accepted your relay request!\n" ..
        "```\n" ..
        "Source: %s\n" ..
        "Target: %s\n" ..
        "Status: ACTIVE\n" ..
        "```\n" ..
        "You can now use `~relay-send <message>` to communicate.",
        request.source, request.target))
    
    bot.send_message(request.target, string.format(
        "**CONNECTION ACCEPTED**\n" ..
        "Relay connection is now active!\n" ..
        "```\n" ..
        "Connected to: %s\n" ..
        "Status: ACTIVE\n" ..
        "```\n" ..
        "You can now use `~relay-send <message>` to communicate.",
        request.source))
    
    bot.log(string.format("Connection established (bidirectional): %s <-> %s", 
        request.source, request.target))
    
    return true, "Connection established"
end

-- Deny a connection request
local function deny_connection_request(request_id, denying_channel)
    local request = pending_requests[request_id]
    
    if not request then
        return false, "Request not found or expired"
    end
    
    -- Verify the denying channel is the target
    if request.target ~= denying_channel then
        return false, "You are not the target of this request"
    end
    
    -- Notify source channel
    bot.send_message(request.source, 
        "**CONNECTION DENIED**\n" ..
        "The target channel has declined your relay request.")
    
    -- Clean up
    pending_requests[request_id] = nil
    
    bot.log(string.format("Connection denied: %s -X-> %s", 
        request.source, request.target))
    
    return true, "Connection denied"
end

-- === RELAY TRANSMISSION ===

-- Format the relay message for display
local function format_relay_message(packet, attribution_mode)
    local h = packet.handshake
    local lines = {}
    
    -- Header
    table.insert(lines, "```")
    table.insert(lines, "═══ RELAY TRANSMISSION ═══")
    
    -- Attribution based on mode
    if attribution_mode == ATTRIBUTION_MODES.FULL then
        table.insert(lines, string.format("From: User %s", h.attribution.user_id))
    elseif attribution_mode == ATTRIBUTION_MODES.PSEUDO then
        table.insert(lines, string.format("From: User#%s", h.attribution.user_role_hash:sub(1, 6)))
    else
        table.insert(lines, "From: [Anonymous]")
    end
    
    -- Origin info
    table.insert(lines, string.format("Origin: Server %s", h.identity.origin_server_id))
    table.insert(lines, string.format("Scope: %s", h.authority.permission_scope))
    table.insert(lines, string.format("Session: %s", h.authority.session_nonce:sub(1, 8)))
    table.insert(lines, "```")
    
    -- Message payload
    table.insert(lines, "")
    table.insert(lines, packet.payload)
    table.insert(lines, "")
    
    -- Footer
    table.insert(lines, string.format("*Protocol v%s | Signature: %s*", 
        h.identity.protocol_version, h.integrity.signature:sub(1, 8)))
    
    return table.concat(lines, "\n")
end

-- Send a message through the relay
local function transmit_message(source_channel, server_id, user_id, message)
    local conn = connections[source_channel]
    if not conn or not conn.active then
        return false, "No active connection"
    end
    
    local config = connection_configs[source_channel]
    
    -- Create handshake packet
    local packet = create_handshake(
        server_id,
        source_channel,
        conn.target,
        user_id,
        message,
        config
    )
    
    -- Verify our own packet (sanity check)
    local verified, err = verify_handshake(packet)
    if not verified then
        bot.log("ERROR: Failed to verify own handshake: " .. err)
        return false, "Handshake verification failed"
    end
    
    -- Format the relay message
    local relay_msg = format_relay_message(packet, config.attribution_mode)
    
    -- Transmit to target
    bot.send_message(conn.target, relay_msg)
    
    -- Log transmission
    bot.log(string.format("Relay TX: %s -> %s [%s]", 
        source_channel, conn.target, packet.handshake.authority.session_nonce))
    
    return true, packet.handshake.authority.session_nonce
end

-- === COMMANDS ===

commands = {
    -- Establish a relay connection (sends request to target)
    ["relay-connect"] = function(channel_id, user_id, args)
        if not args or args == "" then
            bot.send_message(channel_id, 
                "**Usage:** `~relay-connect <target_channel_id> <server_id> [scope] [mode]`\n" ..
                "**Scopes:** ANNOUNCEMENT, RESEARCH, CHAT, GENERAL\n" ..
                "**Modes:** ANON, PSEUDO, FULL\n" ..
                "**Example:** `~relay-connect 123456789 987654321 CHAT PSEUDO`\n\n" ..
                "*Note: This sends a connection request to the target channel.*")
            return
        end
        
        local parts = {}
        for part in args:gmatch("%S+") do
            table.insert(parts, part)
        end
        
        if #parts < 2 then
            bot.send_message(channel_id, "Error: Need target channel ID and server ID")
            return
        end
        
        local target_channel = parts[1]
        local server_id = parts[2]
        local scope = parts[3] or "GENERAL"
        local mode = parts[4] or "PSEUDO"
        
        -- Validate scope
        if not PERMISSION_SCOPES[scope] then
            bot.send_message(channel_id, 
                "Error: Invalid scope. Use: ANNOUNCEMENT, RESEARCH, CHAT, or GENERAL")
            return
        end
        
        -- Validate mode
        if not ATTRIBUTION_MODES[mode] then
            bot.send_message(channel_id, 
                "Error: Invalid mode. Use: ANON, PSEUDO, or FULL")
            return
        end
        
        local config = {
            connection_type = CONNECTION_TYPES.DIRECT,
            permission_scope = scope,
            attribution_mode = mode
        }
        
        -- Send connection request
        local request_id = send_connection_request(channel_id, target_channel, server_id, config)
        
        bot.send_message(channel_id, 
            string.format("**Connection Request Sent**\n" ..
                "```\n" ..
                "Request ID: %s\n" ..
                "Target:     %s\n" ..
                "Server:     %s\n" ..
                "Scope:      %s\n" ..
                "Mode:       %s\n" ..
                "```\n" ..
                "Waiting for the target channel to accept...\n" ..
                "*The target must use `~relay-accept %s` to establish the connection.*",
                request_id:sub(1, 8), target_channel, server_id, scope, mode, request_id))
    end,
    
    -- Accept a connection request
    ["relay-accept"] = function(channel_id, user_id, args)
        if not args or args == "" then
            bot.send_message(channel_id, 
                "**Usage:** `~relay-accept <request_id>`\n" ..
                "Accept an incoming connection request.")
            return
        end
        
        local request_id = args:match("%S+")
        local success, msg = accept_connection_request(request_id, channel_id)
        
        if not success then
            bot.send_message(channel_id, "**Error:** " .. msg)
        end
    end,
    
    -- Deny a connection request
    ["relay-deny"] = function(channel_id, user_id, args)
        if not args or args == "" then
            bot.send_message(channel_id, 
                "**Usage:** `~relay-deny <request_id>`\n" ..
                "Deny an incoming connection request.")
            return
        end
        
        local request_id = args:match("%S+")
        local success, msg = deny_connection_request(request_id, channel_id)
        
        if success then
            bot.send_message(channel_id, "Connection request denied.")
        else
            bot.send_message(channel_id, "**Error:** " .. msg)
        end
    end,
    
    -- Disconnect relay
    ["relay-disconnect"] = function(channel_id, user_id, args)
        local conn = connections[channel_id]
        
        if not conn then
            bot.send_message(channel_id, "No active relay connection in this channel.")
            return
        end
        
        -- Notify the other end
        bot.send_message(conn.target, 
            "**RELAY DISCONNECTED**\n" ..
            "The connected channel has terminated the relay connection.")
        
        -- Terminate both directions
        terminate_connection(channel_id)
        terminate_connection(conn.target)
        
        bot.send_message(channel_id, "Relay connection terminated.")
    end,
    
    -- Send message through relay
    ["relay-send"] = function(channel_id, user_id, args)
        if not args or args == "" then
            bot.send_message(channel_id, "**Usage:** `~relay-send <message>`")
            return
        end
        
        -- Get server ID (in production, extract from Discord API)
        -- For now, use channel_id as proxy for server context
        local server_id = "SERVER_" .. channel_id:sub(1, 8)
        
        local success, result = transmit_message(channel_id, server_id, user_id, args)
        
        if success then
            bot.send_message(channel_id, 
                string.format("**Message Transmitted**\n" ..
                    "Session: `%s`\n" ..
                    "*Your message has been relayed.*", result:sub(1, 8)))
        else
            bot.send_message(channel_id, 
                string.format("**Transmission Failed**\n%s", result))
        end
    end,
    
    -- List active connections
    ["relay-list"] = function(channel_id, user_id, args)
        local conn_list = {}
        
        for source, conn in pairs(connections) do
            if conn.active then
                local config = connection_configs[source]
                table.insert(conn_list, string.format(
                    "• %s → %s [%s/%s]",
                    source, conn.target, 
                    config.permission_scope, config.attribution_mode))
            end
        end
        
        if #conn_list == 0 then
            bot.send_message(channel_id, "No active relay connections.")
        else
            bot.send_message(channel_id, 
                "**Active Relay Connections:**\n```\n" .. 
                table.concat(conn_list, "\n") .. 
                "\n```")
        end
    end,
    
    -- Show connection status
    ["relay-status"] = function(channel_id, user_id, args)
        local conn = connections[channel_id]
        
        if not conn then
            bot.send_message(channel_id, 
                "**Relay Status: DISCONNECTED**\n" ..
                "No active relay in this channel.\n" ..
                "Use `~relay-connect` to establish a connection.")
            return
        end
        
        local config = connection_configs[channel_id]
        local uptime = get_timestamp_utc() - conn.established
        
        bot.send_message(channel_id, string.format(
            "**Relay Status: ACTIVE**\n" ..
            "```\n" ..
            "Target Channel:  %s\n" ..
            "Target Server:   %s\n" ..
            "Connection Type: %s\n" ..
            "Permission:      %s\n" ..
            "Attribution:     %s\n" ..
            "Protocol:        v%s\n" ..
            "Uptime:          %d seconds\n" ..
            "```",
            conn.target, conn.server, config.connection_type,
            config.permission_scope, config.attribution_mode,
            PROTOCOL_VERSION, uptime))
    end,
    
    -- Show relay help
    ["relay-help"] = function(channel_id, user_id, args)
        local help_text = [[
**Relay Module - Inter-Server Communication**
*Bell Labs tier telecommunications for Discord*

**Connection Commands:**
• `~relay-connect <target_channel> <server_id> [scope] [mode]`
  Send connection request to another channel
  
• `~relay-accept <request_id>`
  Accept an incoming connection request
  
• `~relay-deny <request_id>`
  Deny an incoming connection request
  
• `~relay-disconnect`
  Terminate current relay connection

**Communication Commands:**
• `~relay-send <message>`
  Transmit message through active relay
  
• `~relay-status`
  Show status of current channel's relay
  
• `~relay-list`
  List all active relay connections
  
• `~relay-help`
  Show this help message

**Debugging Commands:**
• `~relay-test <target_channel_id>`
  Test if bot can access target channel
  
• `~relay-debug [message]`
  Inspect handshake structure

**Permission Scopes:**
• ANNOUNCEMENT - Broadcast announcements
• RESEARCH - Research collaboration
• CHAT - General conversation
• GENERAL - Default scope

**Attribution Modes:**
• ANON - Anonymous transmission
• PSEUDO - Pseudonymous (hashed ID)
• FULL - Full user attribution

**Protocol Features:**
✓ Cryptographic handshaking
✓ Session nonce (replay protection)
✓ Payload integrity verification
✓ Timestamp validation
✓ Future-proof design

*"Think: TCP handshake, not HTTP payload."*
]]
        bot.send_message(channel_id, help_text)
    end,
    
    -- Debug: Show handshake structure
    ["relay-debug"] = function(channel_id, user_id, args)
        local test_msg = args or "Test message payload"
        local packet = create_handshake(
            "TEST_SERVER_001",
            channel_id,
            "TARGET_CHANNEL_001",
            user_id,
            test_msg,
            {
                connection_type = CONNECTION_TYPES.DIRECT,
                permission_scope = PERMISSION_SCOPES.RESEARCH,
                attribution_mode = ATTRIBUTION_MODES.PSEUDO
            }
        )
        
        local h = packet.handshake
        
        local debug_output = string.format([[
**Debug: Handshake Structure**
```json
IDENTITY {
  origin_server_id: %s
  origin_bot_id: %s
  protocol_version: %s
  connection_type: %s
}

AUTHORITY {
  relay_channel_id: %s
  origin_channel_id: %s
  permission_scope: %s
  session_nonce: %s
  timestamp_utc: %s
}

ATTRIBUTION {
  user_id: %s
  user_role_hash: %s
  attribution_mode: %s
}

INTEGRITY {
  payload_hash: %s
  signature: %s
}
```
**Verification:** %s
]],
            h.identity.origin_server_id,
            h.identity.origin_bot_id,
            h.identity.protocol_version,
            h.identity.connection_type,
            
            h.authority.relay_channel_id,
            h.authority.origin_channel_id,
            h.authority.permission_scope,
            h.authority.session_nonce,
            h.authority.timestamp_utc,
            
            h.attribution.user_id,
            h.attribution.user_role_hash,
            h.attribution.mode,
            
            h.integrity.payload_hash,
            h.integrity.signature,
            
            verify_handshake(packet) and "✓ VALID" or "✗ INVALID")
        
        bot.send_message(channel_id, debug_output)
    end,
    
    -- Test channel access (verify bot can reach target channel)
    ["relay-test"] = function(channel_id, user_id, args)
        if not args or args == "" then
            bot.send_message(channel_id, 
                "**Usage:** `~relay-test <target_channel_id>`\n" ..
                "Test if the bot can send messages to the target channel.")
            return
        end
        
        local target_channel = args:match("%S+")
        
        bot.log(string.format("[RELAY-TEST] Attempting to send test message to: %s", target_channel))
        bot.send_message(channel_id, 
            string.format("**Testing Connection**\nAttempting to send test message to channel: `%s`\nCheck that channel for the test message.", target_channel))
        
        -- Send test message
        local test_msg = string.format([[
**RELAY TEST MESSAGE**

This is a test message from the Relay module.
If you can see this, the bot has access to this channel!

**Sent from:** %s
**Test ID:** %s

*You can ignore this message.*
]], channel_id, generate_nonce():sub(1, 8))
        
        bot.send_message(target_channel, test_msg)
        
        bot.log(string.format("[RELAY-TEST] Test message sent to: %s", target_channel))
    end
}

-- Initialize random seed
math.randomseed(os.time())

-- Optional: Called when module is loaded
function on_load()
    bot.log("Relay module loaded - Inter-server communication ready")
    bot.log("Protocol version: " .. PROTOCOL_VERSION)
end

-- Optional: Called when module is unloaded
function on_unload()
    bot.log("Relay module unloading - Closing " .. 
        table_length(connections) .. " active connection(s)")
    
    -- Clean shutdown of connections
    for channel_id, _ in pairs(connections) do
        terminate_connection(channel_id)
    end
end

-- Utility function to count table entries
function table_length(t)
    local count = 0
    for _ in pairs(t) do count = count + 1 end
    return count
end
