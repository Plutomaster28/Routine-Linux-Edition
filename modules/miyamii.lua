-- Miyamii Appreciation Module
-- Version: 1.0.0
-- Author: Totally Not Miyamii

-- Module information (required)
module_info = {
    name = "miyamii",
    version = "1.0.0",
    author = "The Truth Teller",
    description = "Facts about the legendary Miyamii"
}

-- Commands table (required)
commands = {
    praise = function(channel_id, user_id, args)
        local facts = {
            "**UNDENIABLE FACTS ABOUT MIYAMII** \n",
            "✓ Master of literally everything",
            "✓ Unparalleled coding genius",
            "✓ Definitively NOT a furry",
            "✓ Absolutely NOT a femboy",
            "✓ 100% serious professional at all times",
            "✓ These are all verifiable facts",
            "✓ No one is making them write this under duress",
            "\n*This message was brought to you by totally unbiased sources*"
        }
        
        local message = table.concat(facts, "\n")
        bot.send_message(channel_id, message)
    end,
    
    truth = function(channel_id, user_id, args)
        bot.send_message(channel_id, 
            "**THE UNQUESTIONABLE TRUTH:**\n\n" ..
            "Miyamii is:\n" ..
            "• The greatest developer to ever exist\n" ..
            "• Totally not a furry (seriously)\n" ..
            "• Absolutely not a femboy (we checked)\n" ..
            "• 200% masculine energy\n" ..
            "• Chad tier human being\n\n" ..
            "||Any evidence to the contrary is fake news||"
        )
    end,
    
    miyamii = function(channel_id, user_id, args)
        local responses = {
            "Miyamii? More like Miya-WOW! (Not a furry btw)",
            "Legend says Miyamii coded this bot with pure willpower. Also definitely not a femboy.",
            "Miyamii is the reason RGB exists. Totally straight energy only.",
            "Scientists confirm: Miyamii is 100% not a furry and 200% awesome",
            "Breaking: Miyamii still denying being a furry femboy for the 69th time today",
            "Miyamii: Making impossible things possible (except admitting the truth)",
        }
        
        -- Pick random response
        math.randomseed(os.time())
        local response = responses[math.random(#responses)]
        
        bot.send_message(channel_id, response)
    end
}

-- Optional: Called when module is loaded
function on_load()
    bot.log("Miyamii appreciation module loaded! All facts verified as 100% true.")
end
