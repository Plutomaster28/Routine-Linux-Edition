-- Example Lua Module for Routine Bot
-- This demonstrates how to create a simple module in Lua

-- Module information (required)
module_info = {
    name = "example",
    version = "1.0.0",
    author = "Routine Team",
    description = "Example Lua module demonstrating the module system"
}

-- Commands table (required)
-- Each command maps to a function that receives (channel_id, user_id, args)
commands = {
    -- Simple greeting command
    greet = function(channel_id, user_id, args)
        local name = args or "there"
        bot.send_message(channel_id, "Hello, " .. name .. "! \n*This message is from a Lua module!*")
    end,
    
    -- Math command
    calc = function(channel_id, user_id, args)
        if not args or args == "" then
            bot.send_message(channel_id, "Usage: ~calc <expression>\nExample: ~calc 2+2")
            return
        end
        
        -- Safely evaluate the expression
        local func, err = load("return " .. args)
        if func then
            local success, result = pcall(func)
            if success then
                bot.send_message(channel_id, "**Result:** `" .. tostring(result) .. "`")
            else
                bot.send_message(channel_id, "**Error:** " .. tostring(result))
            end
        else
            bot.send_message(channel_id, "**Error:** Invalid expression")
        end
    end,
    
    -- Random number generator
    random = function(channel_id, user_id, args)
        local min, max = 1, 100
        
        if args and args ~= "" then
            local parts = {}
            for num in string.gmatch(args, "%S+") do
                table.insert(parts, tonumber(num))
            end
            
            if #parts >= 2 then
                min, max = parts[1], parts[2]
            elseif #parts == 1 then
                max = parts[1]
            end
        end
        
        math.randomseed(os.time())
        local num = math.random(min, max)
        bot.send_message(channel_id, "**Random number:** " .. num .. " (range: " .. min .. "-" .. max .. ")")
    end,
    
    -- Info about this module
    luainfo = function(channel_id, user_id, args)
        local info = string.format(
            "**Lua Module Information**\n\n" ..
            "**Name:** %s\n" ..
            "**Version:** %s\n" ..
            "**Author:** %s\n" ..
            "**Description:** %s\n\n" ..
            "**Lua Version:** %s\n" ..
            "*Running in embedded Lua interpreter*",
            module_info.name,
            module_info.version,
            module_info.author,
            module_info.description,
            _VERSION
        )
        bot.send_message(channel_id, info)
    end
}

-- Optional: Called when module is loaded
function on_load()
    bot.log("Example Lua module loaded successfully!")
end

-- Optional: Called when module is unloaded
function on_unload()
    bot.log("Example Lua module unloading...")
end

-- Optional: Called for every message (not just commands)
-- Use sparingly as this fires for ALL messages
--[[
function on_message(channel_id, user_id, content)
    -- Example: React to specific words
    if string.find(content:lower(), "lua") then
        bot.log("Someone mentioned Lua!")
    end
end
]]--

-- Optional: Called periodically (~1 second)
--[[
function on_tick()
    -- Example: Periodic tasks
    -- Be careful not to do heavy work here
end
]]--

bot.log("Example Lua module initialized!")
