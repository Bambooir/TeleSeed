local function run(msg, matches)
	if #matches < 2 then
		return "Ø¨Ø¹Ø¯ Ø§Ø² Ø§ÛŒÙ† Ø¯Ø³ØªÙˆØ±ØŒ Ø¨Ø§ Ù‚ÛŒØ¯ ÛŒÚ© ÙØ§ØµÙ„Ù‡ Ú©Ù„Ù…Ù‡ ÛŒØ§ Ø¬Ù…Ù„Ù‡ ÛŒ Ù…ÙˆØ±Ø¯ Ù†Ø¸Ø± Ø±Ø§ Ø¬Ù‡Øª Ø²ÛŒØ¨Ø§ Ù†ÙˆÛŒØ³ÛŒ ÙˆØ§Ø±Ø¯ Ú©Ù†ÛŒØ¯"
	end
	if string.len(matches[2]) > 20 then
		return "Ø­Ø¯Ø§Ú©Ø«Ø± Ø­Ø±ÙˆÙ Ù…Ø¬Ø§Ø² 20 Ú©Ø§Ø±Ø§Ú©ØªØ± Ø§Ù†Ú¯Ù„ÛŒØ³ÛŒ Ùˆ Ø¹Ø¯Ø¯ Ø§Ø³Øª"
	end
	local font_base = "Ø¶,Øµ,Ù‚,Ù,Øº,Ø¹,Ù‡,Ø®,Ø­,Ø¬,Ø´,Ø³,ÛŒ,Ø¨,Ù„,Ø§,Ù†,Øª,Ù…,Ú†,Ø¸,Ø·,Ø²,Ø±,Ø¯,Ù¾,Ùˆ,Ú©,Ú¯,Ø«,Ú˜,Ø°,Ø¢,Ø¦,.,_"

	local font_hash = "Ø¶,Øµ,Ù‚,Ù,Øº,Ø¹,Ù‡,Ø®,Ø­,Ø¬,Ø´,Ø³,ÛŒ,Ø¨,Ù„,Ø§,Ù†,Øª,Ù…,Ú†,Ø¸,Ø·,Ø²,Ø±,Ø¯,Ù¾,Ùˆ,Ú©,Ú¯,Ø«,Ú˜,Ø°,Ø¢,Ø¦,.,_"
	local fonts = {
		
"Ø¶,Øµ,Ù€á“†Ù€,Ù€Ø‹Ù€,Øº,Ø¹,Ù‡,Ù€×¤Ö¿Ù€,Ù€×¤Ù€,Ø¬,Ø´,Ù€Ï‰Ù€,ÛŒ,Ø¨,Ù„,Ø§,Ù†,Øª,Ù…,Ú†,Ø¸,Ø·,Ø²,Ø±,Ù€×‘,Ù¾,Ù€×¤Ö¼Ù€,Úª,Ú¯,Ø«,Ú˜,Ø°,Ø¢,Ø¦,.,_",
		
"Ø¶,Øµ,Ù€á“†Ù€,Ù€á“…Ù€,Øº,Ø¹,Ù‡Ù€Ù€,Ù€á“˜Ù€,Ù€á“—Ù€,Ø¬,Ø´,Ø³,ÛŒÙ€Ù€,Ø¨,Ù„,Ø§,Ù†,Øª,Ù…,Ú†,Ø¸,Ø·,Ø²,Ø±,Ù€×›,Ù¾,Ù€×¤Ö¼Ù€,ÚªÙ‹Ù€,Ú¯,Ø«,Ú˜,Ø°,Ø¢,Ø¦,.,_",
		
"Ø¶,Øµ,Ù€á“†Ù€,Ù€á“…Ù€,Øº,Ø¹,Ù‡,Ù€á“˜Ù€,Ù€á‘Ù€,Ø¬,Ø´,Ø³,ÛŒ,Ø¨,Ù„,Ø§,Ù†,Øª,Ù€á“„Ù€,Ú†,Ø¸,Ø·,Ø²,Ø±,Ù€×›,Ù¾,Ù€×¤Ö¼Ù€,ÚªÙ‹Ù€,Ú¯,Ø«,Ú˜,Ø°,Ø¢,Ø¦,.,_",
		
"Ø¶,Øµ,Ù‚,Ù€Ø‹Ù€,Øº,Ø¹,Ù‡Ù€Ù€,Ù€á“˜Ù€,Ù€×¤Ù€,Ø¬,Ø´,Ù€Ï‰Ù€,ÛŒ,Ø¨,Ù„,Ø§,Ù†,Øª,Ù…,Ú†,Ø¸,Ø·,Ø²,Ø±,Ù€×›,Ù¾,Ù€×¤Ö¼Ù€,Úª,Ú¯,Ø«,Ú˜,Ø°,Ø¢,Ø¦,.,_",
		
"Ø¶,Øµ,Ù€á“†Ù€,Ù€á“…Ù€,Øº,Ø¹,Ù‡Ù€Ù€,Ù€×¤Ö¿Ù€,Ù€á“—Ù€,Ø¬,Ø´,Ø³,ÛŒÙ€Ù€,Ø¨,Ù„,Ø§,Ù†,Øª,Ù€á“„Ù€,Ú†,Ø¸,Ø·,Ø²,Ø±,Ù€×‘,Ù¾,Ù€×¤Ö¼Ù€,Úª,Ú¯,Ø«,Ú˜,Ø°,Ø¢,Ø¦,.,_",
		
"Ø¶Ù€Ù€,ØµÙ€Ù€,Ù‚Ù€Ù€,ÙÙ€Ù€,ØºÙ€Ù€,Ø¹Ù€Ù€,Ù€Ù‡Ù€Ù€,Ø®Ù€Ù€,Ø­Ù€Ù€,Ø¬Ù€Ù€,Ø´Ù€Ù€, Ø³Ù€,ÛŒÙ€Ù€,Ø¨Ù€Ù€,Ù„Ù€Ù€,ïº‚,Ù†Ù€Ù€,ØªÙ€Ù€,Ù…Ù€Ù€,Ú†Ù€Ù€,Ø¸Ù€Ù€,Ø·Ù€Ù€,Ù€Ø²,Ù€Ø±,Ù€Ø¯,Ù¾Ù€Ù€,Ù€Ùˆ,Ú©Ù€Ù€,Ú¯Ù€Ù€,Ù€Ø«Ù€Ù€,Ù€Ú˜,Ù€Ø°,ïº‚,Ø¦,.,_",		

"Ø¶,Øµ,Ù‚,Ù,Øº,Ø¹,Ù‡,Ø®,Ø­,Ø¬,Ø´,Ø³,ÛŒ,Ø¨,Ù„,Ø§,Ù†,Øª,Ù…,Ú†,Ø¸,Ø·,Ø²,Ø±,Ø¯,Ù¾,Ùˆ,Ú©,Ú¯,Ø«,Ú˜,Ø°,Ø¢,Ø¦,.,_",
	}
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	local result = {}
	i=0
	for k=1,#fonts do
		i=i+1
		local tar_font = fonts[i]:split(",")
		local text = matches[2]
		local text = text:gsub("Ø¶",tar_font[1])
		local text = text:gsub("Øµ",tar_font[2])
		local text = text:gsub("Ù‚",tar_font[3])
		local text = text:gsub("Ù",tar_font[4])
		local text = text:gsub("Øº",tar_font[5])
		local text = text:gsub("Ø¹",tar_font[6])
		local text = text:gsub("Ù‡",tar_font[7])
		local text = text:gsub("Ø®",tar_font[8])
		local text = text:gsub("Ø­",tar_font[9])
		local text = text:gsub("Ø¬",tar_font[10])
		local text = text:gsub("Ø´",tar_font[11])
		local text = text:gsub("Ø³",tar_font[12])
		local text = text:gsub("ÛŒ",tar_font[13])
		local text = text:gsub("Ø¨",tar_font[14])
		local text = text:gsub("Ù„",tar_font[15])
		local text = text:gsub("Ø§",tar_font[16])
		local text = text:gsub("Ù†",tar_font[17])
		local text = text:gsub("Øª",tar_font[18])
		local text = text:gsub("Ù…",tar_font[19])
		local text = text:gsub("Ú†",tar_font[20])
		local text = text:gsub("Ø¸",tar_font[21])
		local text = text:gsub("Ø·",tar_font[22])
		local text = text:gsub("Ø²",tar_font[23])
		local text = text:gsub("Ø±",tar_font[24])
		local text = text:gsub("Ø¯",tar_font[25])
		local text = text:gsub("Ù¾",tar_font[26])
		local text = text:gsub("Ùˆ",tar_font[27])
		local text = text:gsub("Ú©",tar_font[28])
		local text = text:gsub("Ú¯",tar_font[29])
		local text = text:gsub("Ø«",tar_font[30])
		local text = text:gsub("Ú˜",tar_font[31])
		local text = text:gsub("Ø°",tar_font[32])
		local text = text:gsub("Ø¦",tar_font[33])
		local text = text:gsub("Ø¢",tar_font[34])

		table.insert(result, text)
	end
	local result_text = "Ú©Ù„Ù…Ù‡ ÛŒ Ø§ÙˆÙ„ÛŒÙ‡: "..matches[2].."\nØ·Ø±Ø§Ø­ÛŒ Ø¨Ø§ "..tostring(#fonts).." ÙÙˆÙ†Øª:\n______________________________\n"
	a=0
	for v=1,#result do
		a=a+1
		result_text = result_text..a.."- "..result[a].."\n\n"
	end
	return result_text.."______________________________\n@BeyondTeam"
end

return {
	description = "iranian Writer",
	usage = {"font [text] : Ø²ÛŒØ¨Ø§ Ù†ÙˆÛŒØ³ÛŒ",},
	patterns = {
		"^[/!#]([Ff]ont) (.*)",
		"^[/!#]([Ff]ont)$",
		},
	run = run
}
