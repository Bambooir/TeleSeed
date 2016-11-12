local pairs, table, tostring, select = pairs, table, tostring, select
local type, assert, error = type, assert, error
local string = require "string"
local math = require "math"
local Url, Qs
local isLuaNode

if process then
	Url = require "luanode.url"
	Qs = require "luanode.querystring"
	isLuaNode = true
else
	Url = require "socket.url"
end

module((...))

--
-- Encodes the key-value pairs of a table according the application/x-www-form-urlencoded content type.
url_encode_arguments = (isLuaNode and Qs.url_encode_arguments) or function(arguments)
	local body = {}
	for k,v in pairs(arguments) do
		body[#body + 1] = Url.escape(tostring(k)) .. "=" .. Url.escape(tostring(v))
	end
	return table.concat(body, "&")
end


---
-- Multipart form-data helper.
--
-- Taken from https://github.com/catwell/lua-multipart-post
--
do	-- Create a scope to avoid these local helpers to escape

local function fmt(p, ...)
	if select('#',...) == 0 then
		return p
	else
		return string.format(p, ...)
	end
end

local function tprintf(t, p, ...)
	t[#t + 1] = fmt(p, ...)
end

local function append_data(r, k, data, extra)
	tprintf(r, "content-disposition: form-data; name=\"%s\"", k)
	if extra.filename then
		tprintf(r, "; filename=\"%s\"", extra.filename)
	end
	if extra.content_type then
		tprintf(r, "\r\ncontent-type: %s", extra.content_type)
	end
	if extra.content_transfer_encoding then
		tprintf(r, "\r\ncontent-transfer-encoding: %s", extra.content_transfer_encoding)
	end
	tprintf(r, "\r\n\r\n")
	tprintf(r, data)
	tprintf(r, "\r\n")
end

local function gen_boundary()
	local t = {"BOUNDARY-"}
	for i = 2, 17 do
		t[i] = string.char(math.random(65, 90))
	end
	t[18] = "-BOUNDARY"
	return table.concat(t)
end

local function encode(t, boundary)
	local r = {}
	local _t, _tkey, key
	
	-- generate a boundary if none was supplied
	boundary = boundary or gen_boundary()
	
	for k,v in pairs(t) do
		tprintf(r,"--%s\r\n",boundary)

		_tkey, _t = type(k), type(v)

		if _tkey ~= "string" and _tkey ~= "number" and _tkey ~= "boolean" then
			return nil, ("unexpected type %s for key %s"):format(_tkey, tostring(k))
		end

		if _t == "string" then
			append_data(r, k, v, {})
		elseif _t == "number" or _t == "boolean" then
			append_data(r, k, tostring(v), {})
		elseif _t == "table" then
			assert(v.data, "invalid input")
			local extra = {
				filename = v.filename or v.name,
				content_type = v.content_type or v.mimetype or "application/octet-stream",
				content_transfer_encoding = v.content_transfer_encoding or "binary",
			}
			append_data(r, k, v.data, extra)
		else
			return nil, ("unexpected type %s for value at key %s"):format(_t, tostring(k))
		end
	end
	tprintf(r, "--%s--\r\n", boundary)
	return table.concat(r)
end


multipart = {
	---
	-- t is a table with the data to be encoded as multipart/form-data
	-- TODO: improve docs
	Request = function(t)
		local boundary = gen_boundary()
		local body, err = encode(t, boundary)
		if not body then
			return body, err
		end
		return {
			body = body,
			headers = {
				["Content-Length"] = #body,
				["Content-Type"] = ("multipart/form-data; boundary=%s"):format(boundary),
			},
		}
	end
}

end	-- end of multipart scope
