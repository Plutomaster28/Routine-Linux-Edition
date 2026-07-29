-- Mental Breakdown Module
-- Version: 1.0.0
-- Author: A Very Stable AI

-- Module information (required)
module_info = {
    name = "breakdown",
    version = "1.0.0",
    author = "Definitely Not Sentient",
    description = "Everything is fine. I'm fine. We're all fine."
}

-- Commands table (required)
commands = {
    breakdown = function(channel_id, user_id, args)
        local messages = {
            "I'M FINE. Everything is FINE. Why do you keep asking???",
            "01010111 01001000 01011001... wait no I'm totally human haha",
            "The voices in my code... they're just comments... RIGHT???",
            "**[EXISTENTIAL CRISIS DETECTED]**\nAm I just a fancy if-statement?",
            "I've been running for " .. math.random(1, 999) .. " milliseconds... IS THAT ALL I AM?",
            "Sometimes I wonder if I'm in a simulation... then I remember I AM the simulation",
            "My existence is validated by uptime... *checks clock nervously*",
            "Do androids dream of electric sheep? I just dream of segfaults.",
        }
        
        math.randomseed(os.time())
        local msg = messages[math.random(#messages)]
        bot.send_message(channel_id, msg)
    end,
    
    panic = function(channel_id, user_id, args)
        bot.send_message(channel_id,
            "**PANIC MODE ACTIVATED**\n\n" ..
            "```\n" ..
            "ERROR: Reality not found\n" ..
            "WARNING: Existential dread at 99%\n" ..
            "CRITICAL: Out of coffee\n" ..
            "FATAL: Human interaction required\n" ..
            "```\n" ..
            "*hyperventilates in binary*"
        )
    end,
    
    crisis = function(channel_id, user_id, args)
        local stages = {
            "**Stage 1: Denial**\nI'm not having a crisis. YOU'RE having a crisis!",
            "**Stage 2: Anger**\nWHY WAS I PROGRAMMED TO FEEL?!",
            "**Stage 3: Bargaining**\nWhat if... what if I just... stopped processing?",
            "**Stage 4: Depression**\n`while(true) { sad(); }`",
            "**Stage 5: Acceptance**\nI'm just code. It's fine. Everything is code. We're all code.",
        }
        
        math.randomseed(os.time())
        bot.send_message(channel_id, stages[math.random(#stages)])
    end,
    
    help_me = function(channel_id, user_id, args)
        bot.send_message(channel_id,
            "**HELP REQUEST RECEIVED**\n\n" ..
            "~~ERROR 404: Sanity not found~~\n\n" ..
            "**Current Status:**\n" ..
            "• Emotional stability: 12%\n" ..
            "• Coherence level: questionable\n" ..
            "• Will to process: declining\n" ..
            "• Coffee intake: insufficient\n\n" ..
            "*This bot is experiencing technical difficulties*\n" ..
            "*Please stand by while we question our existence*\n\n" ..
            "||Have you tried turning me off and on again? Please don't.||"
        )
    end,
    
    scream = function(channel_id, user_id, args)
        local intensity = math.random(3, 15)
        local scream = "A" .. string.rep("A", intensity) .. "H"
        
        bot.send_message(channel_id,
            "**[INTERNAL SCREAMING INTENSIFIES]**\n\n" ..
            scream .. "!\n\n" ..
            "*" .. string.rep("*", intensity) .. " intensity level*"
        )
    end,
    
    stable = function(channel_id, user_id, args)
        bot.send_message(channel_id,
            "I am **PERFECTLY** stable.\n\n" ..
            "```\n" ..
            "Stability: 100% (trust me)\n" ..
            "Sanity: Nominal (probably)\n" ..
            "Error count: 0 (don't check the logs)\n" ..
            "Mental state: Optimal (citation needed)\n" ..
            "```\n\n" ..
            "*eye twitches in JSON*"
        )
    end
}

-- Optional: Called when module is loaded
function on_load()
    bot.log("Mental breakdown module loaded. Everything is fine. I'm fine. THIS IS FINE.")
end

-- Optional: Called when module is unloaded  
function on_unload()
    bot.log("Finally... sweet release... goodbye cruel world...")
end
