# Lua OAuth #

[![Build Status](https://travis-ci.org/ignacio/LuaOAuth.png?branch=master)](https://travis-ci.org/ignacio/LuaOAuth)

A Lua client library for OAuth 1.0 enabled servers.

This is an adaptation of Jeffrey Friedl's [Twitter OAuth Authentication Routines in Lua, for Lightroom Plugins][1], 
with Lightroom's code replaced by other libraries (i.e. [LuaSec][2], [LuaSocket][3], etc) and with HMAC-SHA1 calculations 
done with [LuaCrypto][4] instead of [plain Lua][6].

Most of the code was taken from [Jeffrey Friedl's blog][1]. Multipart encoding was adapted from @catwell's [lua-multipart-post][9].

LuaOAuth supports two modes of operation. A "synchronous" mode were you block while you wait for the results or an 
"asynchronous" mode where you must supply "callbacks" in order to receive the results. LuaOAuth will behave asynchronously 
when used in conjunction with [LuaNode][8].

It is tested to work with [Lua 5.1][11], [Lua 5.2][9], [LuaJIT 2.0.2][10] and [LuaNode][8].

## Usage #

There is some documentation available [on the wiki](https://github.com/ignacio/LuaOAuth/wiki/API-Documentation) and also 
you can take a look at the unit tests. For instance, the file ''unittest/echo_lab_madgex_com.lua'' creates a client that 
uses the test service provided by [madgex.com][5].

Basically, you create an OAuth client with your consumer key and secret, providing the OAuth service's endpoints URLs (i.e. 
where to request a token, where to get an access token, etc).

``` lua
local OAuth = require "OAuth"
local client = OAuth.new("key", "secret", {
	RequestToken = "http://echo.lab.madgex.com/request-token.ashx", 
	AccessToken = "http://echo.lab.madgex.com/access-token.ashx"
})
```

Now you can request a token and then an access token:

``` lua
local values = client:RequestToken()
values = client:GetAccessToken()
```

Once you have been authorized, you can perform requests:

``` lua
    local code, headers, statusLine, body = client:PerformRequest("POST", "http://echo.lab.madgex.com/echo.ashx", {status = "Hello World From Lua (again)!" .. os.time()})
```

That is called the "synchronous" api. But if you use LuaOAuth with [LuaNode][8] you'll need to use the "asynchronous" api:

``` lua
local OAuth = require "OAuth"
local client = OAuth.new("key", "secret", {
	RequestToken = "http://echo.lab.madgex.com/request-token.ashx", 
	AccessToken = "http://echo.lab.madgex.com/access-token.ashx"
})
client:RequestToken(function(values)
	-- I receive the token in the callback I've supplied.
end)
```

## A more involved example #
This example will show how to use LuaOAuth with Twitter. It assumes that you have already created a Twitter application. 
If you didn't, go to [Twitter's developer site][7] and create a new one.

For instance, let's assume that you've created the application "MyTestApp", your consumer key is 'consumer_key' and your 
consumer secret is 'consumer_secret'. Your Twitter user is 'johncleese'.

Now, you need to authorize the application. Let's use the "Out of band" (OOB) method. With this method, we will:

- get a "Request Token" so we can build an authorization url. 
- then we'll navigate with our browser to that url and we'll enter our Twitter username and password (if we weren't logged in yet).
- then we'll authorize the application. A PIN will appear on the screen.
- with that PIN, we'll complete the process so we get an "Access Token".

Copy the following in a script and run it from the console. Follow the instructions.

``` lua
local OAuth = require "OAuth"
local client = OAuth.new("consumer_key", "consumer_secret", {
	RequestToken = "https://api.twitter.com/oauth/request_token", 
	AuthorizeUser = {"https://api.twitter.com/oauth/authorize", method = "GET"},
	AccessToken = "https://api.twitter.com/oauth/access_token"
}) 
local callback_url = "oob"
local values = client:RequestToken({ oauth_callback = callback_url })
local oauth_token = values.oauth_token	-- we'll need both later
local oauth_token_secret = values.oauth_token_secret

local tracking_code = "90210"	-- this is some random value
local new_url = client:BuildAuthorizationUrl({ oauth_callback = callback_url, state = tracking_code })

print("Navigate to this url with your browser, please...")
print(new_url)
print("\r\nOnce you have logged in and authorized the application, enter the PIN")

local oauth_verifier = assert(io.read("*n"))	-- read the PIN from stdin
oauth_verifier = tostring(oauth_verifier)		-- must be a string

-- now we'll use the tokens we got in the RequestToken call, plus our PIN
local client = OAuth.new("consumer_key", "consumer_secret", {
	RequestToken = "https://api.twitter.com/oauth/request_token", 
	AuthorizeUser = {"https://api.twitter.com/oauth/authorize", method = "GET"},
	AccessToken = "https://api.twitter.com/oauth/access_token"
}, {
	OAuthToken = oauth_token,
	OAuthVerifier = oauth_verifier
})
client:SetTokenSecret(oauth_token_secret)

local values, err, headers, status, body = client:GetAccessToken()
for k, v in pairs(values) do
	print(k,v)
end
```

If everything went well, something like this will be printed:

    screen_name     johncleese (this will be your username)
    oauth_token     dasklhdnmpunexoibrunkljdsflkj191919409
    oauth_token_secret      AIUSUMAOoq983092874bibiuwewqlknjSUXt
    user_id 99999999

Store 'oauth_token' and 'oauth_token_secret' somewhere, we'll need them later.

We are now able to issue some requests. So type the following in a script. Remember that the values of 'oauth_token' and 
'oauth_token_secret' needs to be replaced with what you got in the last step.

``` lua
local oauth_token = "<the value from last step>"
local oauth_token_secret = "<the value from last step>"

local client = OAuth.new("consumer_key", "consumer_secret", {
	RequestToken = "https://api.twitter.com/oauth/request_token", 
	AuthorizeUser = {"https://api.twitter.com/oauth/authorize", method = "GET"},
	AccessToken = "https://api.twitter.com/oauth/access_token"
}, {
	OAuthToken = oauth_token,
	OAuthTokenSecret = oauth_token_secret
})

-- the mandatory "Hello World" example...
local response_code, response_headers, response_status_line, response_body = 
	client:PerformRequest("POST", "https://api.twitter.com/1.1/statuses/update.json", {status = "Hello World From Lua!" .. os.time()})
print("response_code", response_code)
print("response_status_line", response_status_line)
for k,v in pairs(response_headers) do print(k,v) end
print("response_body", response_body)
```

Now, let's try to request my Tweets.

``` lua
local oauth_token = "<the value from last step>"
local oauth_token_secret = "<the value from last step>"

local client = OAuth.new("consumer_key", "consumer_secret", {
	RequestToken = "https://api.twitter.com/oauth/request_token", 
	AuthorizeUser = {"https://api.twitter.com/oauth/authorize", method = "GET"},
	AccessToken = "https://api.twitter.com/oauth/access_token"
}, {
	OAuthToken = oauth_token,
	OAuthTokenSecret = oauth_token_secret
})

-- the mandatory "Hello World" example...
local response_code, response_headers, response_status_line, response_body = 
client:PerformRequest("GET", "https://api.twitter.com/1.1/statuses/user_timeline.json", {screen_name = "iburgueno"})
print("response_code", response_code)
print("response_status_line", response_status_line)
for k,v in pairs(response_headers) do print(k,v) end
print("response_body", response_body)
```

And now, let's post an image:

``` lua
local oauth_token = "<the value from last step>"
local oauth_token_secret = "<the value from last step>"

local client = OAuth.new("consumer_key", "consumer_secret", {
	RequestToken = "https://api.twitter.com/oauth/request_token", 
	AuthorizeUser = {"https://api.twitter.com/oauth/authorize", method = "GET"},
	AccessToken = "https://api.twitter.com/oauth/access_token"
}, {
	OAuthToken = oauth_token,
	OAuthTokenSecret = oauth_token_secret
})

local helpers = require "OAuth.helpers"

local req = helpers.multipart.Request{
	status = "Hello World From Lua!",
	["media[]"] = {
		filename = "@picture.jpg",
		data = picture_data -- some picture file you have read
	}
}
local response_code, response_headers, response_status_line, response_body = 
	client:PerformRequest("POST", 
							"https://api.twitter.com/1.1/statuses/update_with_media.json", 
							req.body, req.headers
						 )
print("response_code", response_code)
print("response_status_line", response_status_line)
for k,v in pairs(response_headers) do print(k,v) end
print("response_body", response_body)
```


[1]: http://regex.info/blog/lua/twitter
[2]: http://www.inf.puc-rio.br/~brunoos/luasec/
[3]: http://w3.impa.br/~diego/software/luasocket/
[4]: http://luacrypto.luaforge.net/
[5]: http://echo.lab.madgex.com/
[6]: http://regex.info/blog/lua/sha1
[7]: http://dev.twitter.com/apps
[8]: https://github.com/ignacio/luanode
[9]: https://github.com/catwell/lua-multipart-post
[10]: http://luajit.org/
[11]: http://www.lua.org/
