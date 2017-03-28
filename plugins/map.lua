do
local api_key = nil
local base_api = "https://maps.googleapis.com/maps/api"
local function get_staticmap(area)
	local api = base_api.."/staticmap?"
	local lat,lng,acc,types = get_latlong(area)
	local scale = types[1]
	if scale=="locality" then
		zoom=8
	elseif scale=="country" then
		zoom=4
	else
		zoom = 13
	end
	local parameters = "size=600x300"
		.."&zoom="..zoom
		.."&center="..URL.escape(area)
		.."&markers=color:red"..URL.escape("|"..area)
	if api_key ~=nil and api_key ~= "" then
		parameters = parameters.."&key="..api_key
	end
	return lat, lng, api..parameters
end

local function run(msg, matches)
	local receiver = get_receiver(msg)
	local lat,lng,url = get_staticmap(matches[2])
	if matches[1] == "loc" then
		send_location(receiver, lat, lng, ok_cb, false)
	elseif matches[1] == "map" then
		local zooms = {10, 16}
		local urls = {}
		for i = 1, #zooms do
			local zoom = zooms[i]
			local url = "https://maps.googleapis.com/maps/api/staticmap?zoom=" .. zoom .. "&size=600x300&maptype=roadmap&center=" .. lat .. "," .. lng .. "&markers=color:blue%7Clabel:X%7C" .. lat .. "," .. lng
			table.insert(urls, url)
		end
		send_photos_from_url(receiver, urls)
	elseif matches[1] == "view" then
		local zooms = {12, 18}
		local urls = {}
		for i = 1, #zooms do
			local zoom = zooms[i]
			local url = "nhttps://maps.googleapis.com/maps/api/staticmap?zoom=" .. zoom .. "&size=600x300&maptype=hybrid&center=" .. lat .. "," .. lng .. "&markers=color:blue%7Clabel:X%7C" .. lat .. "," .. lng
			table.insert(urls, url)
		end
		send_photos_from_url(receiver, urls)
	elseif matches[1] == "link" then
		return "موقعيت مکاني در گوگل مپ:\nhttps://www.google.com/maps/place/" .. lat .. "," .. lng
	elseif matches[1] == "gps" then
		return "مختصات محل مورد نظر:\n"..lat..","..lng
	end
	return nil
end

return {
	description = "Get Map Location by Name", 
	usage = {
		"map loc (name) : لوکيشن",
		"map link (name) : لينک گوگل مپ",
		"map map (name) : نقشه",
		"map view (name) : تصوير واقعي",
		"map gps (name) : مختصات",
	},
	patterns = {
		"^[Mm]ap (loc) (.*)$",
		"^[Mm]ap (map) (.*)$",
		"^[Mm]ap (view) (.*)$",
		"^[Mm]ap (link) (.*)$",
		"^[Mm]ap (gps) (.*)$",
	}, 
	run = run 
}
end
