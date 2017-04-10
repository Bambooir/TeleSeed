do
      local sys_messages = {
        downloading = "در حال دريافت ويدئو...",
        uploading = "ويدئو شما در حال بارگذاري است...",
  error = "خطا در دريافت ويدئو",
  not_found ="ويدئو پيدا نشد",
  maxFilesize = 0,
      }
   local STATUS_ERROR =0
   local STATUS_404 =1
   local STATUS_SUCCESS =2
      local function dl_video(link,tag)
  local video = 'aparat_' .. tag .. '.mp4'
  local path = '/tmp/' .. video .. ' '
        run_command('[ -f ' .. path .. '] || curl -o ' .. path .. link )
  local result = run_command('[ -f ' .. path .. '] || echo "error"')
        if result == 'error' then return nil else return video end
      end
   local function get_video_link(tag)
  local url = "http://aparat.com/v/" .. tag
  local file_pattern = 
'<meta%sproperty="og%:video%:url"%scontent="([:/_A-Za-z0-9-.]+.mp4)">'
  local link = ""
  local res,code = http.request(url)
  if code ~= 200 then return STATUS_ERROR,nil end
  link =string.match(res,file_pattern)
  if link == "" or link== nil then return STATUS_404,link end
  return STATUS_SUCCESS,link
   end
      local function send_video_to(receiver,video_file)
        send_large_msg(receiver,sys_messages.uploading)
        file = '/tmp/' .. video_file
        local cb_extra = {file_path=file}
 print(file)
        --send_document(receiver, file, rmtmp_cb, cb_extra)
 send_video(receiver, file, rmtmp_cb, cb_extra)
      end
      local function run(msg, matches)
        local receiver = get_receiver(msg)
  local status,link = get_video_link(matches[1])
  if status == STATUS_ERROR then return sys_messages.error end
  if status == STATUS_404 then return sys_messages.not_found end
  local video = dl_video(link,matches[1])
  if video == nil or video == "" then return sys_messages.error end
  send_video_to(receiver,video)
      end
        return {
        --description = "دانلود فيلم از سايت يوتيوب",
  description = "دانلود فیلم از آپارات",
        usage = "بصورت اتوماتیک لینک آپارات را تشخیص داده و فیلم را دانلود میکند",
        patterns = {
          "[/#!][Aa]paratdl http://aparat.com/v/([_A-Za-z0-9-]+)/?"
        },
        run = run
      }
end
