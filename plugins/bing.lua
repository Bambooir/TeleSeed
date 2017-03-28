local function bing(msg,special)
local url = https.request("https://irapi.ir/bing/api.php?txt="..URL.escape(special[1]))
local jdat = json:decode(url)
local text = ""
text = text.."عنوان : "..jdat.text1.."\nلینک : "..jdat.link1.."\n\n"
text = text.."عنوان : "..jdat.text2.."\nلینک : "..jdat.link2.."\n\n"
text = text.."عنوان : "..jdat.text3.."\nلینک : "..jdat.link3.."\n\n"
text = text.."عنوان : "..jdat.text4.."\nلینک : "..jdat.link4.."\n\n"
text = text.."عنوان : "..jdat.text5.."\nلینک : "..jdat.link5.."\n\n"
text = text.."عنوان : "..jdat.text6.."\nلینک : "..jdat.link6.."\n\n"
text = text.."عنوان : "..jdat.text7.."\nلینک : "..jdat.link7.."\n\n"
text = text.."عنوان : "..jdat.text8.."\nلینک : "..jdat.link8.."\n\n"
text = text.."عنوان : "..jdat.text9.."\nلینک : "..jdat.link9.."\n\n"
text = text.."عنوان : "..jdat.text10.."\nلینک : "..jdat.link10.."\n\n"
return text
end

return {
patterns = { "^[#!/][Bb]ing (.*)$"},
run = bing
}
--[[
plugin by sudo1
from specialteam
@special_programming
api from: https://irapi.ir
special thank,s (crul team)]]
