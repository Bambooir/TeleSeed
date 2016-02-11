do
umbrella = "http://umbrella.shayan-soft.ir/quran/" -- database

-- get sound of sura
local function read_sura(chat_id, target)
 local readq = http.request(umbrella.."Sura"..target..".mp3")
 local url = umbrella.."Sura"..target..".mp3"
 local file = download_to_file(url)
 local cb_extra = {file_path=file}
 return send_document("chat#id"..chat_id, file, rmtmp_cb, cb_extra)
end

-- get text of sura
local function view_sura(chat_id, target)
 local viewq = http.request(umbrella.."quran ("..target..").txt")
 return viewq
end

-- run script
local function run(msg, matches)
    local chat_id = msg.to.id
 
 if matches[1] == "read" then
  local target = matches[2]
  return read_sura(chat_id, target)
  
 elseif matches[1] == "sura" then
  local target = matches[2]
  return view_sura(chat_id, target)
  
 elseif matches [1] == "quran" then
  local qlist = http.request(umbrella.."list.txt") -- list of suras
  return qlist
  
 end
end

-- other help and commands
return {
 description = "Umbrella Quran Project", 
 usage = {
  "!sura (num) : view arabic sura",
  "!read (num) : send sound of sura",
  "!quran : sura list of quran",
  },
 patterns = {
  "^[!/](sura) (.+)$",
  "^[!/](read) (.+)$",
  "^[!/](quran)$",
  }, 
 run = run,
 }
end

-- thanks to shayan
