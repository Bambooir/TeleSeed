local Ltn12 = require "ltn12"
local Http = require "socket.http"
local Https = require "ssl.https"
local helpers = require "OAuth.helpers"

local pairs,type, error, tostring = pairs,type, error, tostring
local table = table

module((...))

-- 
-- Performs the actual http request, using LuaSocket or LuaSec (when using an https scheme)
-- @param url is the url to request
-- @param method is the http method (GET, POST, etc)
-- @param headers are the supplied http headers as a table
-- @param arguments is an optional table with whose keys and values will be encoded as "application/x-www-form-urlencoded" 
--   or a string (or something that can be converted to a string). In that case, you must supply the Content-Type.
-- @param post_body is a string with all parameters (custom + oauth ones) encoded. This is used when the OAuth provider 
--   does not support the 'Authorization' header.
function PerformRequestHelper (self, url, method, headers, arguments, post_body)
	-- arguments have already been sanitized
	
	-- this method screams "refactor me!"
	local response_body = {}
	local request_constructor = {
		url = url,
		method = method,
		headers = headers,
		sink = Ltn12.sink.table(response_body),
		redirect = false
	}
	
	if method == "PUT" then
		if type(arguments) == "table" then
			error("unsupported table argument for PUT")
		else
			local string_data = tostring(arguments)
			if string_data == "nil" then
				error("data must be something convertible to a string")
			end
			request_constructor.headers["Content-Length"] = tostring(#string_data)
			request_constructor.source = Ltn12.source.string(string_data)
		end
	
	elseif method == "POST" then
		if type(arguments) == "table" then
			request_constructor.headers["Content-Type"] = "application/x-www-form-urlencoded"
			if not self.m_supportsAuthHeader then
				-- send all parameters (oauth + custom) in the body
				request_constructor.headers["Content-Length"] = tostring(#post_body)
				request_constructor.source = Ltn12.source.string(post_body)
			else
				-- encode the custom parameters and send them in the body
				local source = helpers.url_encode_arguments(arguments)
				request_constructor.headers["Content-Length"] = tostring(#source)
				request_constructor.source = Ltn12.source.string(source)
			end
		elseif arguments then
			if not self.m_supportsAuthHeader then
				error("can't send POST body if the server does not support 'Authorization' header")
			end
			local string_data = tostring(arguments)
			if string_data == "nil" then
				error("data must be something convertible to a string")
			end
			request_constructor.headers["Content-Length"] = tostring(#string_data)
			request_constructor.source = Ltn12.source.string(string_data)
		else
			request_constructor.headers["Content-Length"] = "0"
		end
		
	elseif method == "GET" or method == "HEAD" or method == "DELETE" then
		if self.m_supportsAuthHeader then
			if arguments then
				request_constructor.url = url .. "?" .. helpers.url_encode_arguments(arguments)
			end
		else
			-- send all parameters (oauth + custom) in the url
			request_constructor.url = url .. "?" .. post_body
		end
	end
	
	local ok, response_code, response_headers, response_status_line
	if url:match("^https://") then
		ok, response_code, response_headers, response_status_line = Https.request(request_constructor)
	elseif url:match("^http://") then
		ok, response_code, response_headers, response_status_line = Http.request(request_constructor)
	else
		error( ("unsupported scheme '%s'"):format( tostring(url:match("^([^:]+)")) ) )
	end
	
	if not ok then
		return nil, response_code, response_headers, response_status_line, response_body
	end
	
	response_body = table.concat(response_body)
	
	--[=[
	for k,v in pairs(response_headers or {}) do
		print( ("%s: %s"):format(k,v) )
	end
	print( ("response: %s"):format(response_body) )
	--]=]
	
	return true, response_code, response_headers, response_status_line, response_body
end