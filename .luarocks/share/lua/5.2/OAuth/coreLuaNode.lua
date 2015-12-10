local Http = require "luanode.http"
local Url = require "luanode.url"

local helpers = require "OAuth.helpers"
local console = require "luanode.console"

local pairs,type, error, tostring = pairs,type, error, tostring
local table = table

module((...))

--- 
-- Performs the actual http request, using LuaSocket or LuaSec (when using an https scheme)
-- @param url is the url to request
-- @param method is the http method (GET, POST, etc)
-- @param headers are the supplied http headers as a table
-- @param arguments is an optional table with whose keys and values will be encoded as "application/x-www-form-urlencoded" 
--   or a string (or something that can be converted to a string). In that case, you must supply the Content-Type.
-- @param post_body is a string with all parameters (custom + oauth ones) encoded. This is used when the OAuth provider 
--   does not support the 'Authorization' header.
-- @param callback is a function to be called with the results of the request when they're available. The callback 
--   receives the following arguments: an (optional) error object, http status code, http response headers, 
--   http status line and the response body
--   In case of a connection error (host unreacheable, etc), the callback will be called with 
--       { message = <error message>, 
--         code = <error code>
--       }
--
function PerformRequestHelper (self, url, method, headers, arguments, post_body, callback)
	-- arguments have already been sanitized
	
	-- this method screams "refactor me!"
	local parsedUrl = Url.parse(url)
	
	local request_constructor = {
		url = url,
		method = method,
		headers = headers,
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
			request_constructor.source = string_data
		end
	
	elseif method == "POST" then
		if type(arguments) == "table" then
			request_constructor.headers["Content-Type"] = "application/x-www-form-urlencoded"
			if not self.m_supportsAuthHeader then
				-- send all parameters (oauth + custom) in the body
				request_constructor.headers["Content-Length"] = tostring(#post_body)
				request_constructor.source = post_body
			else
				-- encode the custom parameters and send them in the body
				local source = helpers.url_encode_arguments(arguments)
				request_constructor.headers["Content-Length"] = tostring(#source)
				request_constructor.source = source
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
			request_constructor.source = string_data
		else
			request_constructor.headers["Content-Length"] = "0"
		end
		
	elseif method == "GET" or method == "HEAD" or method == "DELETE" then
		if self.m_supportsAuthHeader then
			if arguments then
				request_constructor.url = parsedUrl.path .. "?" .. helpers.url_encode_arguments(arguments)
			end
		else
			-- send all parameters (oauth + custom) in the url
			request_constructor.url = parsedUrl.path .. "?" .. post_body
		end
	end
	
	local secure = false
		
	if parsedUrl.protocol == "https:" then
		secure = true
		parsedUrl.port = parsedUrl.port or 443
	elseif parsedUrl.protocol == "http:" then
		secure = false
		parsedUrl.port = parsedUrl.port or 80
	else
		error( ("unsupported scheme '%s'"):format( parsedUrl.protocol ) )
	end

	local client = Http.createClient(parsedUrl.port, parsedUrl.host, secure)
	
	client:on("error", function(self, err_msg, err_code)
		callback({ code = err_code, message = err_msg })
	end)
	
	-- this sucks!
	request_constructor.headers["Host"] = parsedUrl.host
	
	local request = client:request(method, request_constructor.url, request_constructor.headers)
	
	request.__buffer = {}
	
	request:on("response", function(self, response)
		
		response:on("data", function(self, data)
			request.__buffer[#request.__buffer + 1] = data
		end)
		
		response:on("end", function(self)
			callback(nil, response.statusCode, response.headers, tostring(response.statusCode), table.concat(request.__buffer))
		end)
	end)
	
	if request_constructor.source then
		request:finish(request_constructor.source)
	else
		request:finish()
	end

	
	return true
end
