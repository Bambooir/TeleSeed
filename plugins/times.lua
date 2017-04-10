local function run(msg, matches)
local url , res = http.request('http://api.gpmod.ir/time/')
local jdat = json:decode(url)
local url = "http://latex.codecogs.com/png.download?".."\\dpi{800}%20\\LARGE%20"..jdat.ENtime
local file = download_to_file(url,'DeaD.webp')
send_document(get_receiver(msg) , file, ok_cb, false)
end
 
return {
  patterns = {
    "^زمان$"
  }, 
  run = run 
}
