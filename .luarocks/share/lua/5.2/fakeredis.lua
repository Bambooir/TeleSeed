local unpack = table.unpack or unpack

--- Bit operations

local ok,bit
if _VERSION == "Lua 5.3" then
  bit = (load [[ return {
    band = function(x, y) return x & y end,
    bor = function(x, y) return x | y end,
    bxor = function(x, y) return x ~ y end,
    bnot = function(x) return ~x end,
    rshift = function(x, n) return x >> n end,
    lshift = function(x, n) return x << n end,
  } ]])()
else
  ok,bit = pcall(require,"bit")
  if not ok then bit = bit32 end
end

assert(type(bit) == "table", "module for bitops not found")

--- default sleep

local default_sleep
do
  local ok, mod = pcall(require, "socket")
  if ok and type(mod) == "table" then
    default_sleep = mod.sleep
  else
    default_sleep = function(n)
      local t0 = os.clock()
      while true do
        local delta = os.clock() - t0
        if (delta < 0) or (delta > n) then break end
      end
    end
  end
end

--- Helpers

local xdefv = function(ktype)
  if ktype == "list" then
    return {head = 0, tail = 0}
  elseif ktype == "zset" then
    return {
      list = {},
      set = {},
    }
  else return {} end
end

local xgetr = function(self, k, ktype)
  if self.data[k] then
    assert(
      (self.data[k].ktype == ktype),
      "ERR Operation against a key holding the wrong kind of value"
    )
    assert(self.data[k].value)
    return self.data[k].value
  else return xdefv(ktype) end
end

local xgetw = function(self, k, ktype)
  if self.data[k] and self.data[k].value then
    assert(
      (self.data[k].ktype == ktype),
      "ERR Operation against a key holding the wrong kind of value"
    )
  else
    self.data[k] = {ktype = ktype, value = xdefv(ktype)}
  end
  return self.data[k].value
end

local empty = function(self, k)
  local v, t = self.data[k].value, self.data[k].ktype
  if t == nil then
    return true
  elseif t == "string" then
    return not v[1]
  elseif (t == "hash") or (t == "set") then
    for _,_ in pairs(v) do return false end
    return true
  elseif t == "list" then
    return v.head == v.tail
  elseif t == "zset" then
    if #v.list == 0 then
      for _,_ in pairs(v.set) do error("incoherent") end
      return true
    else
      for _,_ in pairs(v.set) do return(false) end
      error("incoherent")
    end
  else error("unsupported") end
end

local cleanup = function(self, k)
  if empty(self, k) then self.data[k] = nil end
end

local is_integer = function(x)
  return (type(x) == "number") and (math.floor(x) == x)
end

local overflows = function(n)
  return (n > 2^53-1) or (n < -2^53+1)
end

local is_bounded_integer = function(x)
  return (is_integer(x) and (not overflows(x)))
end

local is_finite_number = function(x)
  return (type(x) == "number") and (x > -math.huge) and (x < math.huge)
end

local toint = function(x)
  if type(x) == "string" then x = tonumber(x) end
  return is_bounded_integer(x) and x or nil
end

local tofloat = function(x)
  if type(x) == "number" then return x end
  if type(x) ~= "string" then return nil end
  local r = tonumber(x)
  if r then return r end
  if x == "inf" or x == "+inf" then
    return math.huge
  elseif x == "-inf" then
    return -math.huge
  else return nil end
end

local tostr = function(x)
  if is_bounded_integer(x) then
    return string.format("%d", x)
  else return tostring(x) end
end

local char_bitcount = function(x)
  assert(
    (type(x) == "number") and
    (math.floor(x) == x) and
    (x >= 0) and (x < 256)
  )
  local n = 0
  while x ~= 0 do
    x = bit.band(x, x-1)
    n = n+1
  end
  return n
end

local chkarg = function(x)
  if type(x) == "number" then x = tostr(x) end
  assert(type(x) == "string")
  return x
end

local chkargs = function(n, ...)
  local arg = {...}
  assert(#arg == n)
  for i=1,n do arg[i] = chkarg(arg[i]) end
  return unpack(arg)
end

local getargs = function(...)
  local arg = {...}
  local n = #arg; assert(n > 0)
  for i=1,n do arg[i] = chkarg(arg[i]) end
  return arg
end

local getargs_as_map = function(...)
  local arg, r = getargs(...), {}
  assert(#arg%2 == 0)
  for i=1,#arg,2 do r[arg[i]] = arg[i+1] end
  return r
end

local chkargs_wrap = function(f, n)
  assert( (type(f) == "function") and (type(n) == "number") )
  return function(self, ...) return f(self, chkargs(n, ...)) end
end

local lset_to_list = function(s)
  local r = {}
  for v,_ in pairs(s) do r[#r+1] = v end
  return r
end

local nkeys = function(x)
  local r = 0
  for _,_ in pairs(x) do r = r + 1 end
  return r
end

--- Commands

-- keys

local del = function(self, ...)
  local arg = getargs(...)
  local r = 0
  for i=1,#arg do
    if self.data[arg[i]] then r = r + 1 end
    self.data[arg[i]] = nil
  end
  return r
end

local exists = function(self, k)
  return not not self.data[k]
end

local keys = function(self, pattern)
  assert(type(pattern) == "string")
  -- We want to convert the Redis pattern to a Lua pattern.
  -- Start by escaping dashes *outside* character classes.
  -- We also need to escape percents here.
  local t, p, n = {}, 1, #pattern
  local p1, p2
  while true do
    p1, p2 = pattern:find("%[.+%]", p)
    if p1 then
      if p1 > p then
        t[#t+1] = {true, pattern:sub(p, p1-1)}
      end
      t[#t+1] = {false, pattern:sub(p1, p2)}
      p = p2+1
      if p > n then break end
    else
      t[#t+1] = {true, pattern:sub(p, n)}
      break
    end
  end
  for i=1,#t do
    if t[i][1] then
      t[i] = t[i][2]:gsub("[%%%-]", "%%%0")
    else t[i] = t[i][2]:gsub("%%", "%%%%") end
  end
  -- Remaining Lua magic chars are: '^$().[]*+?' ; escape them except '*?[]'
  -- Then convert '\' to '%', '*' to '.*' and '?' to '.'. Leave '[]' as is.
  -- Wrap in '^$' to enforce bounds.
  local lp = "^" .. table.concat(t):gsub("[%^%$%(%)%.%+]", "%%%0")
    :gsub("\\", "%%"):gsub("%*", ".*"):gsub("%?", ".") .. "$"
  local r = {}
  for k,_ in pairs(self.data) do
    if k:match(lp) then r[#r+1] = k end
  end
  return r
end

local _type = function(self, k)
  return self.data[k] and self.data[k].ktype or "none"
end

local randomkey = function(self)
  local ks = lset_to_list(self.data)
  local n = #ks
  if n > 0 then
    return ks[math.random(1, n)]
  else return nil end
end

local rename = function(self, k, k2)
  assert((k ~= k2) and self.data[k])
  self.data[k2] = self.data[k]
  self.data[k] = nil
  return true
end

local renamenx = function(self, k, k2)
  if self.data[k2] then
    return false
  else
    return rename(self, k, k2)
  end
end

-- strings

local getrange, incrby, set

local append = function(self, k, v)
  local x = xgetw(self, k, "string")
  x[1] = (x[1] or "") .. v
  return #x[1]
end

local bitcount = function(self, k, i1, i2)
  k = chkarg(k)
  local s
  if i1 or i2 then
    assert(i1 and i2, "ERR syntax error")
    s = getrange(self, k, i1, i2)
  else
    s = xgetr(self, k, "string")[1] or ""
  end
  local r, bytes = 0,{s:byte(1, -1)}
  for i=1,#bytes do
    r = r + char_bitcount(bytes[i])
  end
  return r
end

local bitop = function(self, op, k, ...)
  assert(type(op) == "string")
  op = op:lower()
  assert(
    (op == "and") or
    (op == "or") or
    (op == "xor") or
    (op == "not"),
    "ERR syntax error"
  )
  k = chkarg(k)
  local arg = {...}
  local good_arity = (op == "not") and (#arg == 1) or (#arg > 0)
  assert(good_arity, "ERR wrong number of arguments for 'bitop' command")
  local l, vals = 0, {}
  local s
  for i=1,#arg do
    s = xgetr(self, arg[i], "string")[1] or ""
    if #s > l then l = #s end
    vals[i] = s
  end
  if l == 0 then
    del(self, k)
    return 0
  end
  local vector_mt = {__index=function() return 0 end}
  for i=1,#vals do
    vals[i] = setmetatable({vals[i]:byte(1, -1)}, vector_mt)
  end
  local r = {}
  if op == "not" then
    assert(#vals[1] == l)
    for i=1,l do
      r[i] = bit.band(bit.bnot(vals[1][i]), 0xff)
    end
  else
    local _op = bit["b" .. op]
    for i=1,l do
      local t = {}
      for j=1,#vals do t[j] = vals[j][i] end
      r[i] = _op(unpack(t))
    end
  end
  set(self, k, string.char(unpack(r)))
  return l
end

local decr = function(self, k)
  return incrby(self, k, -1)
end

local decrby = function(self, k, n)
  n = toint(n)
  assert(n, "ERR value is not an integer or out of range")
  return incrby(self, k, -n)
end

local get = function(self, k)
  local x = xgetr(self, k, "string")
  return x[1]
end

local getbit = function(self, k, offset)
  k = chkarg(k)
  offset = toint(offset)
  assert(
    (offset >= 0),
    "ERR bit offset is not an integer or out of range"
  )
  local bitpos = offset % 8 -- starts at 0
  local bytepos = (offset - bitpos) / 8 -- starts at 0
  local s = xgetr(self, k, "string")[1] or ""
  if bytepos >= #s then return 0 end
  local char = s:sub(bytepos+1, bytepos+1):byte()
  return bit.band(bit.rshift(char, 7-bitpos), 1)
end

getrange = function(self, k, i1, i2)
  k = chkarg(k)
  i1, i2 = toint(i1), toint(i2)
  assert(i1 and i2)
  local x = xgetr(self, k, "string")
  x = x[1] or ""
  if i1 >= 0 then i1 = i1 + 1 end
  if i2 >= 0 then i2 = i2 + 1 end
  return x:sub(i1, i2)
end

local getset = function(self, k, v)
  local r = get(self, k)
  set(self, k, v)
  return r
end

local incr = function(self, k)
  return incrby(self, k, 1)
end

incrby = function(self, k, n)
  k, n = chkarg(k), toint(n)
  assert(n, "ERR value is not an integer or out of range")
  local x = xgetw(self, k, "string")
  local i = toint(x[1] or 0)
  assert(i, "ERR value is not an integer or out of range")
  i = i+n
  assert(
    (not overflows(i)),
    "ERR increment or decrement would overflow"
  )
  x[1] = tostr(i)
  return i
end

local incrbyfloat = function(self, k, n)
  k, n = chkarg(k), tofloat(n)
  assert(n, "ERR value is not a valid float")
  local x = xgetw(self, k, "string")
  local i = tofloat(x[1] or 0)
  assert(i, "ERR value is not a valid float")
  i = i+n
  assert(
    is_finite_number(i),
    "ERR increment would produce NaN or Infinity"
  )
  x[1] = tostr(i)
  return i
end

local mget = function(self, ...)
  local arg, r = getargs(...), {}
  for i=1,#arg do r[i] = get(self, arg[i]) end
  return r
end

local mset = function(self, ...)
  local argmap = getargs_as_map(...)
  for k,v in pairs(argmap) do set(self, k, v) end
  return true
end

local msetnx = function(self, ...)
  local argmap = getargs_as_map(...)
  for k,_ in pairs(argmap) do
    if self.data[k] then return false end
  end
  for k,v in pairs(argmap) do set(self, k, v) end
  return true
end

set = function(self, k, v)
  self.data[k] = {ktype = "string", value = {v}}
  return true
end

local setbit = function(self, k, offset, b)
  k = chkarg(k)
  offset, b = toint(offset), toint(b)
  assert(
    (offset >= 0),
    "ERR bit offset is not an integer or out of range"
  )
  assert(
    (b == 0) or (b == 1),
    "ERR bit is not an integer or out of range"
  )
  local bitpos = offset % 8 -- starts at 0
  local bytepos = (offset - bitpos) / 8 -- starts at 0
  local s = xgetr(self, k, "string")[1] or ""
  local pad = {s}
  for i=2,bytepos+2-#s do pad[i] = "\0" end
  s = table.concat(pad)
  assert(#s >= bytepos+1)
  local before = s:sub(1, bytepos)
  local char = s:sub(bytepos+1, bytepos+1):byte()
  local after = s:sub(bytepos+2, -1)
  local old = bit.band(bit.rshift(char, 7-bitpos), 1)
  if b == 1 then
    char = bit.bor(bit.lshift(1, 7-bitpos), char)
  else
    char = bit.band(bit.bnot(bit.lshift(1, 7-bitpos)), char)
  end
  local r = before .. string.char(char) .. after
  set(self, k, r)
  return old
end

local setnx = function(self, k, v)
  if self.data[k] then
    return false
  else
    return set(self, k, v)
  end
end

local setrange = function(self, k, i, s)
  local k, s = chkargs(2, k, s)
  i = toint(i)
  assert(i and (i >= 0))
  local x = xgetw(self, k, "string")
  local y = x[1] or ""
  local ly, ls = #y, #s
  if i > ly then -- zero padding
    local t = {}
    for i=1, i-ly do t[i] = "\0" end
    y = y .. table.concat(t) .. s
  else
    y = y:sub(1, i) .. s .. y:sub(i+ls+1, ly)
  end
  x[1] = y
  return #y
end

local strlen = function(self, k)
  local x = xgetr(self, k, "string")
  return x[1] and #x[1] or 0
end

-- hashes

local hdel = function(self, k, ...)
  k = chkarg(k)
  local arg = getargs(...)
  local r = 0
  local x = xgetw(self, k, "hash")
  for i=1,#arg do
    if x[arg[i]] then r = r + 1 end
    x[arg[i]] = nil
  end
  cleanup(self, k)
  return r
end

local hget
local hexists = function(self, k, k2)
  return not not hget(self, k, k2)
end

hget = function(self, k, k2)
  local x = xgetr(self, k, "hash")
  return x[k2]
end

local hgetall = function(self, k)
  local x = xgetr(self, k, "hash")
  local r = {}
  for _k,v in pairs(x) do r[_k] = v end
  return r
end

local hincrby = function(self, k, k2, n)
  k, k2, n = chkarg(k), chkarg(k2), toint(n)
  assert(n, "ERR value is not an integer or out of range")
  assert(type(n) == "number")
  local x = xgetw(self, k, "hash")
  local i = toint(x[k2] or 0)
  assert(i, "ERR value is not an integer or out of range")
  i = i+n
  assert(
    (not overflows(i)),
    "ERR increment or decrement would overflow"
  )
  x[k2] = tostr(i)
  return i
end

local hincrbyfloat = function(self, k, k2, n)
  k, k2, n = chkarg(k), chkarg(k2), tofloat(n)
  assert(n, "ERR value is not a valid float")
  local x = xgetw(self, k, "hash")
  local i = tofloat(x[k2] or 0)
  assert(i, "ERR value is not a valid float")
  i = i+n
  assert(
    is_finite_number(i),
    "ERR increment would produce NaN or Infinity"
  )
  x[k2] = tostr(i)
  return i
end

local hkeys = function(self, k)
  local x = xgetr(self, k, "hash")
  local r = {}
  for _k,_ in pairs(x) do r[#r+1] = _k end
  return r
end

local hlen = function(self, k)
  local x = xgetr(self, k, "hash")
  return nkeys(x)
end

local hmget = function(self, k, k2s)
  k = chkarg(k)
  assert((type(k2s) == "table"))
  local r = {}
  local x = xgetr(self, k, "hash")
  for i=1,#k2s do r[i] = x[chkarg(k2s[i])] end
  return r
end

local hmset = function(self, k, ...)
  k = chkarg(k)
  local arg = {...}
  if type(arg[1]) == "table" then
    assert(#arg == 1)
    local x = xgetw(self, k, "hash")
    for _k,v in pairs(arg[1]) do x[chkarg(_k)] = chkarg(v) end
  else
    assert(#arg % 2 == 0)
    local x = xgetw(self, k, "hash")
    local t = getargs(...)
    for i=1,#t,2 do x[t[i]] = t[i+1] end
  end
  return true
end

local hset = function(self, k, k2, v)
  local x = xgetw(self, k, "hash")
  local r = not x[k2]
  x[k2] = v
  return r
end

local hsetnx = function(self, k, k2, v)
  local x = xgetw(self, k, "hash")
  if x[k2] == nil then
    x[k2] = v
    return true
  else
    return false
  end
end

local hvals = function(self, k)
  local x = xgetr(self, k, "hash")
  local r = {}
  for _,v in pairs(x) do r[#r+1] = v end
  return r
end

-- lists (head = left, tail = right)

local _l_real_i = function(x, i)
  if i < 0 then
    return x.tail+i+1
  else
    return x.head+i+1
  end
end

local _l_len = function(x)
  return x.tail - x.head
end

local _block_for = function(self, timeout)
  if timeout > 0 then
    local sleep = self.sleep or default_sleep
    if type(sleep) == "function" then
      sleep(timeout)
    else
      error("sleep function unavailable", 0)
    end
  else
    error("operation would block", 0)
  end
end

local rpoplpush

local blpop = function(self, ...)
  local arg = {...}
  local timeout = toint(arg[#arg])
  arg[#arg] = nil
  local vs = getargs(...)
  local x, l, k, v
  for i=1,#vs do
    k = vs[i]
    x = xgetw(self, k, "list")
    l = _l_len(x)
    if l > 0 then
      v = x[x.head+1]
      if l > 1 then
        x.head = x.head + 1
        x[x.head] = nil
      else self.data[k] = nil end
      return {k, v}
    else self.data[k] = nil end
  end
  _block_for(self, timeout)
end

local brpop = function(self, ...)
  local arg = {...}
  local timeout = toint(arg[#arg])
  arg[#arg] = nil
  local vs = getargs(...)
  local x, l, k, v
  for i=1,#vs do
    k = vs[i]
    x = xgetw(self, k, "list")
    l = _l_len(x)
    if l > 0 then
      v = x[x.tail]
      if l > 1 then
        x[x.tail] = nil
        x.tail = x.tail - 1
      else self.data[k] = nil end
      return {k, v}
    else self.data[k] = nil end
  end
  _block_for(self, timeout)
end

local brpoplpush = function(self, k1, k2, timeout)
  k1, k2 = chkargs(2, k1, k2)
  timeout = toint(timeout)
  if not self.data[k1] then _block_for(self, timeout) end
  return rpoplpush(self, k1, k2)
end

local lindex = function(self, k, i)
  k = chkarg(k)
  i = assert(toint(i))
  local x = xgetr(self, k, "list")
  return x[_l_real_i(x, i)]
end

local linsert = function(self, k, mode, pivot, v)
  mode = mode:lower()
  assert((mode == "before") or (mode == "after"))
  if not self.data[k] then return 0 end
  local x = xgetw(self, k, "list")
  local p = nil
  for i=x.head+1, x.tail do
    if x[i] == pivot then
      p = i
      break
    end
  end
  if not p then return -1 end
  if mode == "after" then
    for i=x.head+1, p do x[i-1] = x[i] end
    x.head = x.head - 1
  else
    for i=x.tail, p, -1 do x[i+1] = x[i] end
    x.tail = x.tail + 1
  end
  x[p] = v
  return _l_len(x)
end

local llen = function(self, k)
  local x = xgetr(self, k, "list")
  return _l_len(x)
end

local lpop = function(self, k)
  local x = xgetw(self, k, "list")
  local l, r = _l_len(x), x[x.head+1]
  if l > 1 then
    x.head = x.head + 1
    x[x.head] = nil
  else self.data[k] = nil end
  return r
end

local lpush = function(self, k, ...)
  local vs = getargs(...)
  local x = xgetw(self, k, "list")
  for i=1,#vs do
    x[x.head] = vs[i]
    x.head = x.head - 1
  end
  return _l_len(x)
end

local lpushx = function(self, k, v)
  if not self.data[k] then return 0 end
  local x = xgetw(self, k, "list")
  x[x.head] = v
  x.head = x.head - 1
  return _l_len(x)
end

local lrange = function(self, k, i1, i2)
  k = chkarg(k)
  i1, i2 = toint(i1), toint(i2)
  assert(i1 and i2)
  local x, r = xgetr(self, k, "list"), {}
  i1 = math.max(_l_real_i(x, i1), x.head+1)
  i2 = math.min(_l_real_i(x, i2), x.tail)
  for i=i1,i2 do r[#r+1] = x[i] end
  return r
end

local _lrem_i = function(x, p)
  for i=p,x.tail do
    x[i] = x[i+1]
  end
  x.tail = x.tail - 1
end

local _lrem_l = function(x, v, s)
  assert(v)
  if not s then s = x.head+1 end
  for i=s,x.tail do
    if x[i] == v then
      _lrem_i(x, i)
      return i
    end
  end
  return false
end

local _lrem_r = function(x, v, s)
  assert(v)
  if not s then s = x.tail end
  for i=s,x.head+1,-1 do
    if x[i] == v then
      _lrem_i(x, i)
      return i
    end
  end
  return false
end

local lrem = function(self, k, count, v)
  k, v = chkarg(k), chkarg(v)
  count = assert(toint(count))
  if not self.data[k] then return 0 end
  local x = xgetw(self, k, "list")
  local n, last = 0, nil
  local op = (count < 0) and _lrem_r or _lrem_l
  local limited = (count ~= 0)
  count = math.abs(count)
  while true do
    last = op(x, v, last)
    if last then
      n = n+1
      if limited then
        count = count - 1
        if count == 0 then break end
      end
    else break end
  end
  return n
end

local lset = function(self, k, i, v)
  k, v = chkarg(k), chkarg(v)
  i = assert(toint(i))
  if not self.data[k] then
    error("ERR no such key")
  end
  local x = xgetw(self, k, "list")
  local l = _l_len(x)
  if i >= l or i < -l then
    error("ERR index out of range")
  end
  x[_l_real_i(x, i)] = v
  return true
end

local ltrim = function(self, k, i1, i2)
  k = chkarg(k)
  i1, i2 = toint(i1), toint(i2)
  assert(i1 and i2)
  local x = xgetw(self, k, "list")
  i1, i2 = _l_real_i(x, i1), _l_real_i(x, i2)
  for i=x.head+1,i1-1 do x[i] = nil end
  for i=i2+1,x.tail do x[i] = nil end
  x.head = math.max(i1-1, x.head)
  x.tail = math.min(i2, x.tail)
  assert(
    (x[x.head] == nil) and
    (x[x.tail+1] == nil)
  )
  cleanup(self, k)
  return true
end

local rpop = function(self, k)
  local x = xgetw(self, k, "list")
  local l, r = _l_len(x), x[x.tail]
  if l > 1 then
    x[x.tail] = nil
    x.tail = x.tail - 1
  else self.data[k] = nil end
  return r
end

rpoplpush = function(self, k1, k2)
  local v = rpop(self, k1)
  if not v then return nil end
  lpush(self, k2, v)
  return v
end

local rpush = function(self, k, ...)
  local vs = getargs(...)
  local x = xgetw(self, k, "list")
  for i=1,#vs do
    x.tail = x.tail + 1
    x[x.tail] = vs[i]
  end
  return _l_len(x)
end

local rpushx = function(self, k, v)
  if not self.data[k] then return 0 end
  local x = xgetw(self, k, "list")
  x.tail = x.tail + 1
  x[x.tail] = v
  return _l_len(x)
end

-- sets

local sadd = function(self, k, ...)
  k = chkarg(k)
  local arg = getargs(...)
  local x, r = xgetw(self, k, "set"), 0
  for i=1,#arg do
    if not x[arg[i]] then
      x[arg[i]] = true
      r = r + 1
    end
  end
  return r
end

local scard = function(self, k)
  local x = xgetr(self, k, "set")
  return nkeys(x)
end

local _sdiff = function(self, k, ...)
  k = chkarg(k)
  local arg = getargs(...)
  local x = xgetr(self, k, "set")
  local r = {}
  for v,_ in pairs(x) do r[v] = true end
  for i=1,#arg do
    x = xgetr(self, arg[i], "set")
    for v,_ in pairs(x) do r[v] = nil end
  end
  return r
end

local sdiff = function(self, k, ...)
  return lset_to_list(_sdiff(self, k, ...))
end

local sdiffstore = function(self, k2, k, ...)
  k2 = chkarg(k2)
  local x = _sdiff(self, k, ...)
  self.data[k2] = {ktype = "set", value = x}
  return nkeys(x)
end

local _sinter = function(self, k, ...)
  k = chkarg(k)
  local arg = getargs(...)
  local x = xgetr(self, k, "set")
  local r = {}
  local y
  for v,_ in pairs(x) do
    r[v] = true
    for i=1,#arg do
      y = xgetr(self, arg[i], "set")
      if not y[v] then r[v] = nil; break end
    end
  end
  return r
end

local sinter = function(self, k, ...)
  return lset_to_list(_sinter(self, k, ...))
end

local sinterstore = function(self, k2, k, ...)
  k2 = chkarg(k2)
  local x = _sinter(self, k, ...)
  self.data[k2] = {ktype = "set", value = x}
  return nkeys(x)
end

local sismember = function(self, k, v)
  local x = xgetr(self, k, "set")
  return not not x[v]
end

local smembers = function(self, k)
  local x = xgetr(self, k, "set")
  return lset_to_list(x)
end

local smove = function(self, k, k2, v)
  local x = xgetr(self, k, "set")
  if x[v] then
    local y = xgetw(self, k2, "set")
    x[v] = nil
    y[v] = true
    return true
  else return false end
end

local spop = function(self, k)
  local x, r = xgetw(self, k, "set"), nil
  local l = lset_to_list(x)
  local n = #l
  if n > 0 then
    r = l[math.random(1, n)]
    x[r] = nil
  end
  cleanup(self, k)
  return r
end

local srandmember = function(self, k, count)
  k = chkarg(k)
  local x = xgetr(self, k, "set")
  local l = lset_to_list(x)
  local n = #l
  if not count then
    if n > 0 then
      return l[math.random(1, n)]
    else return nil end
  end
  count = toint(count)
  if (count == 0) or (n == 0) then return {} end
  if count >= n then return l end
  local r = {}
  if count > 0 then -- distinct elements
    for i=0,count-1 do
      r[#r+1] = table.remove(l, math.random(1, n-i))
    end
  else -- allow repetition
    for i=1,-count do
      r[#r+1] = l[math.random(1, n)]
    end
  end
  return r
end

local srem = function(self, k, ...)
  k = chkarg(k)
  local arg = getargs(...)
  local x, r = xgetw(self, k, "set"), 0
  for i=1,#arg do
    if x[arg[i]] then
      x[arg[i]] = nil
      r = r + 1
    end
  end
  cleanup(self, k)
  return r
end

local _sunion = function(self, ...)
  local arg = getargs(...)
  local r = {}
  local x
  for i=1,#arg do
    x = xgetr(self, arg[i], "set")
    for v,_ in pairs(x) do r[v] = true end
  end
  return r
end

local sunion = function(self, k, ...)
  return lset_to_list(_sunion(self, k, ...))
end

local sunionstore = function(self, k2, k, ...)
  k2 = chkarg(k2)
  local x = _sunion(self, k, ...)
  self.data[k2] = {ktype = "set", value = x}
  return nkeys(x)
end

-- zsets

local _z_p_mt = {
  __eq = function(a, b)
    if a.v == b.v then
      assert(a.s == b.s)
      return true
    else return false end
  end,
  __lt = function(a, b)
    if a.s == b.s then
      return (a.v < b.v)
    else
      return (a.s < b.s)
    end
  end,
}

local _z_pair = function(s, v)
  assert(
    (type(s) == "number") and
    (type(v) == "string")
  )
  local r = {s = s, v = v}
  return setmetatable(r, _z_p_mt)
end

local _z_pairs = function(...)
  local arg = {...}
  assert((#arg > 0) and (#arg % 2 == 0))
  local ps = {}
  for i=1,#arg,2 do
    ps[#ps+1] = _z_pair(
      assert(tofloat(arg[i])),
      chkarg(arg[i+1])
    )
  end
  return ps
end

local _z_insert = function(x, ix, p)
  assert(
    (type(x) == "table") and
    (type(ix) == "number") and
    (type(p) == "table")
  )
  local l = x.list
  table.insert(l, ix, p)
  for i=ix+1,#l do
    x.set[l[i].v] = x.set[l[i].v] + 1
  end
  x.set[p.v] = ix
end

local _z_remove = function(x, v)
  if not x.set[v] then return false end
  local l, ix = x.list, x.set[v]
  assert(l[ix].v == v)
  table.remove(l, ix)
  for i=ix,#l do
    x.set[l[i].v] = x.set[l[i].v] - 1
  end
  x.set[v] = nil
  return true
end

local _z_remove_range = function(x, i1, i2)
  local l = x.list
  i2 = i2 or i1
  assert(
    (i1 > 0) and
    (i2 >= i1) and
    (i2 <= #l)
  )
  local ix, n = i1, i2-i1+1
  for i=1,n do
    x.set[l[ix].v] = nil
    table.remove(l, ix)
  end
  for i=ix,#l do
    x.set[l[i].v] = x.set[l[i].v] - n
  end
  return n
end

local _z_update = function(x, p)
  local l = x.list
  local found = _z_remove(x, p.v)
  local ix = nil
  for i=1,#l do
    if l[i] > p then
      ix = i; break
    end
  end
  if not ix then ix = #l+1 end
  _z_insert(x, ix, p)
  return found
end

local _z_coherence = function(x)
  local l, s = x.list, x.set
  local found, n = {}, 0
  for val,pos in pairs(s) do
    if found[pos] then return false end
    found[pos] = true
    n = n + 1
    if not (l[pos] and (l[pos].v == val)) then
      return false
    end
  end
  if #l ~= n then return false end
  for i=1, n-1 do
    if l[i].s > l[i+1].s then return false end
  end
  return true
end

local _z_normrange = function(l, i1, i2)
  i1, i2 = assert(toint(i1)), assert(toint(i2))
  if i1 < 0 then i1 = #l+i1 end
  if i2 < 0 then i2 = #l+i2 end
  i1, i2 = math.max(i1+1, 1), i2+1
  if (i2 < i1) or (i1 > #l) then return nil end
  i2 = math.min(i2, #l)
  return i1, i2
end

local _zrbs_opts = function(...)
  local arg = {...}
  if #arg == 0 then return {} end
  local ix, opts = 1, {}
  while type(arg[ix]) == "string" do
    if arg[ix] == "withscores" then
      opts.withscores = true
      ix = ix + 1
    elseif arg[ix] == "limit" then
      opts.limit = {
        offset = assert(toint(arg[ix+1])),
        count = assert(toint(arg[ix+2])),
      }
      ix = ix + 3
    else error("input") end
  end
  if type(arg[ix]) == "table" then
    local _o = arg[ix]
    opts.withscores = opts.withscores or _o.withscores
    if _o.limit then
      opts.limit = {
        offset = assert(toint(_o.limit.offset or _o.limit[1])),
        count = assert(toint(_o.limit.count or _o.limit[2])),
      }
    end
    ix = ix + 1
  end
  assert(arg[ix] == nil)
  if opts.limit then
    assert(
      (opts.limit.count >= 0) and
      (opts.limit.offset  >= 0)
    )
  end
  return opts
end

local _z_store_params = function(dest, numkeys, ...)
  dest = chkarg(dest)
  numkeys = assert(toint(numkeys))
  assert(numkeys > 0)
  local arg = {...}
  assert(#arg >= numkeys)
  local ks = {}
  for i=1, numkeys do ks[i] = chkarg(arg[i]) end
  local ix, opts = numkeys+1,{}
  while type(arg[ix]) == "string" do
    if arg[ix] == "weights" then
      opts.weights = {}
      ix = ix + 1
      for i=1, numkeys do
        opts.weights[i] = assert(toint(arg[ix]))
        ix = ix + 1
      end
    elseif arg[ix] == "aggregate" then
      opts.aggregate = assert(chkarg(arg[ix+1]))
      ix = ix + 2
    else error("input") end
  end
  if type(arg[ix]) == "table" then
    local _o = arg[ix]
    opts.weights = opts.weights or _o.weights
    opts.aggregate = opts.aggregate or _o.aggregate
    ix = ix + 1
  end
  assert(arg[ix] == nil)
  if opts.aggregate then
    assert(
      (opts.aggregate == "sum") or
      (opts.aggregate == "min") or
      (opts.aggregate == "max")
    )
  else opts.aggregate = "sum" end
  if opts.weights then
    assert(#opts.weights == numkeys)
    for i=1,#opts.weights do
      assert(type(opts.weights[i]) == "number")
    end
  else
    opts.weights = {}
    for i=1, numkeys do opts.weights[i] = 1 end
  end
  opts.keys = ks
  opts.dest = dest
  return opts
end

local _zrbs_limits = function(x, s1, s2, descending)
  local s1_incl, s2_incl = true, true
  if s1:sub(1, 1) == "(" then
    s1, s1_incl = s1:sub(2, -1), false
  end
  s1 = assert(tofloat(s1))
  if s2:sub(1, 1) == "(" then
    s2, s2_incl = s2:sub(2, -1), false
  end
  s2 = assert(tofloat(s2))
  if descending then
    s1, s2 = s2, s1
    s1_incl, s2_incl = s2_incl, s1_incl
  end
  if s2 < s1 then return nil end
  local l = x.list
  local i1, i2
  local fst, lst = l[1].s, l[#l].s
  if (fst > s2) or ((not s2_incl) and (fst == s2)) then return nil end
  if (lst < s1) or ((not s1_incl) and (lst == s1)) then return nil end
  if (fst > s1) or (s1_incl and (fst == s1)) then i1 = 1 end
  if (lst < s2) or (s2_incl and (lst == s2)) then i2 = #l end
  for i=1,#l do
    if (i1 and i2) then break end
    if (not i1) then
      if l[i].s > s1 then i1 = i end
      if s1_incl and l[i].s == s1 then i1 = i end
    end
    if (not i2) then
      if l[i].s > s2 then i2 = i-1 end
      if (not s2_incl) and l[i].s == s2 then i2 = i-1 end
    end
  end
  assert(i1 and i2)
  if descending then
    return #l-i2, #l-i1
  else
    return i1-1, i2-1
  end
end

local dbg_zcoherence = function(self, k)
  local x = xgetr(self, k, "zset")
  return _z_coherence(x)
end

local zadd = function(self, k, ...)
  k = chkarg(k)
  local ps = _z_pairs(...)
  local x = xgetw(self, k, "zset")
  local n = 0
  for i=1,#ps do
    if not _z_update(x, ps[i]) then n = n+1 end
  end
  return n
end

local zcard = function(self, k)
  local x = xgetr(self, k, "zset")
  return #x.list
end

local zcount = function(self, k, s1, s2)
  local x = xgetr(self, k, "zset")
  local i1, i2 = _zrbs_limits(x, s1, s2, false)
  if not (i1 and i2) then return 0 end
  assert(i2 >= i1)
  return i2 - i1 + 1
end

local zincrby = function(self, k, n, v)
  k,v = chkargs(2, k, v)
  n = assert(tofloat(n))
  local x = xgetw(self, k, "zset")
  local p = x.list[x.set[v]]
  local s = p and (p.s + n) or n
  _z_update(x, _z_pair(s, v))
  return s
end

local zinterstore = function(self, ...)
  local params = _z_store_params(...)
  local x = xdefv("zset")
  local aggregate
  if params.aggregate == "sum" then
    aggregate = function(x, y) return x+y end
  elseif params.aggregate == "min" then
    aggregate = math.min
  elseif params.aggregate == "max" then
    aggregate = math.max
  else error() end
  local y = xgetr(self, params.keys[1], "zset")
  local p1, p2
  for j=1,#y.list do
    p1 = _z_pair(y.list[j].s, y.list[j].v)
    _z_update(x, p1)
  end
  for i=2,#params.keys do
    y = xgetr(self, params.keys[i], "zset")
    local to_remove, to_update = {}, {}
    for j=1,#x.list do
      p1 = x.list[j]
      if y.set[p1.v] then
        p2 = _z_pair(
          aggregate(
            p1.s,
            params.weights[i] * y.list[y.set[p1.v]].s
          ),
          p1.v
        )
        to_update[#to_update+1] = p2
      else
        to_remove[#to_remove+1] = p1.v
      end
    end
    for j=1,#to_remove do _z_remove(x, to_remove[j]) end
    for j=1,#to_update do _z_update(x, to_update[j]) end
  end
  local r = #x.list
  if r > 0 then
    self.data[params.dest] = {ktype = "zset", value = x}
  end
  return r
end

local _zranger = function(descending)
  return function(self, k, i1, i2, opts)
    k = chkarg(k)
    local withscores = false
    if type(opts) == "table" then
      withscores = opts.withscores
    elseif type(opts) == "string" then
      assert(opts:lower() == "withscores")
      withscores = true
    else assert(opts == nil) end
    local x = xgetr(self, k, "zset")
    local l = x.list
    i1, i2 = _z_normrange(l, i1, i2)
    if not i1 then return {} end
    local inc = 1
    if descending then
      i1 = #l - i1 + 1
      i2 = #l - i2 + 1
      inc = -1
    end
    local r = {}
    if withscores then
      for i=i1, i2, inc do r[#r+1] = {l[i].v, l[i].s} end
    else
      for i=i1, i2, inc do r[#r+1] = l[i].v end
    end
    return r
  end
end

local zrange = _zranger(false)
local zrevrange = _zranger(true)

local _zrangerbyscore = function(descending)
  return function(self, k, s1, s2, ...)
    k, s1, s2 = chkargs(3, k, s1, s2)
    local opts = _zrbs_opts(...)
    local x = xgetr(self, k, "zset")
    local i1, i2 = _zrbs_limits(x, s1, s2, descending)
    if not (i1 and i2) then return {} end
    if opts.limit then
      if opts.limit.count == 0 then return {} end
      i1 = i1 + opts.limit.offset
      if i1 > i2 then return {} end
      i2 = math.min(i2, i1+opts.limit.count-1)
    end
    if descending then
      return zrevrange(self, k, i1, i2, opts)
    else
      return zrange(self, k, i1, i2, opts)
    end
  end
end

local zrangebyscore = _zrangerbyscore(false)
local zrevrangebyscore = _zrangerbyscore(true)

local zrank = function(self, k, v)
  local x = xgetr(self, k, "zset")
  local r = x.set[v]
  if r then
    return r-1
  else return nil end
end

local zrem = function(self, k, ...)
  k = chkarg(k)
  local arg = getargs(...)
  local x, r = xgetw(self, k, "zset"), 0
  for i=1,#arg do
    if _z_remove(x, arg[i]) then r = r + 1 end
  end
  cleanup(self, k)
  return r
end

local zremrangebyrank = function(self, k, i1, i2)
  k = chkarg(k)
  local x = xgetw(self, k, "zset")
  i1, i2 = _z_normrange(x.list, i1, i2)
  if not i1 then
    cleanup(self, k)
    return 0
  end
  local n = _z_remove_range(x, i1, i2)
  cleanup(self, k)
  return n
end

local zremrangebyscore = function(self, k, s1, s2)
  local x = xgetr(self, k, "zset")
  local i1, i2 = _zrbs_limits(x, s1, s2, false)
  if not (i1 and i2) then return 0 end
  assert(i2 >= i1)
  return zremrangebyrank(self, k, i1, i2)
end

local zrevrank = function(self, k, v)
  local x = xgetr(self, k, "zset")
  local r = x.set[v]
  if r then
    return #x.list-r
  else return nil end
end

local zscore = function(self, k, v)
  local x = xgetr(self, k, "zset")
  local p = x.list[x.set[v]]
  if p then
    return p.s
  else return nil end
end

local zunionstore = function(self, ...)
  local params = _z_store_params(...)
  local x = xdefv("zset")
  local default_score, aggregate
  if params.aggregate == "sum" then
    default_score = 0
    aggregate = function(x, y) return x+y end
  elseif params.aggregate == "min" then
    default_score = math.huge
    aggregate = math.min
  elseif params.aggregate == "max" then
    default_score = -math.huge
    aggregate = math.max
  else error() end
  local y, p1, p2
  for i=1,#params.keys do
    y = xgetr(self, params.keys[i], "zset")
    for j=1,#y.list do
      p1 = y.list[j]
      p2 = _z_pair(
        aggregate(
          params.weights[i] * p1.s,
          x.set[p1.v] and x.list[x.set[p1.v]].s or default_score
        ),
        p1.v
      )
      _z_update(x, p2)
    end
  end
  local r = #x.list
  if r > 0 then
    self.data[params.dest] = {ktype = "zset", value = x}
  end
  return r
end

-- connection

local echo = function(self, v)
  return v
end

local ping = function(self)
  return true
end

-- server

local flushdb = function(self)
  self.data = {}
  return true
end

--- Class

local methods = {
  -- keys
  del = del, -- (...) -> #removed
  exists = chkargs_wrap(exists, 1), -- (k) -> exists?
  keys = keys, -- (pattern) -> list of keys
  ["type"] = chkargs_wrap(_type, 1), -- (k) -> [string|list|set|zset|hash|none]
  randomkey = randomkey, -- () -> [k|nil]
  rename = chkargs_wrap(rename, 2), -- (k,k2) -> true
  renamenx = chkargs_wrap(renamenx, 2), -- (k,k2) -> ! existed? k2
  -- strings
  append = chkargs_wrap(append, 2), -- (k,v) -> #new
  bitcount = bitcount, -- (k,[start,end]) -> n
  bitop = bitop, -- ([and|or|xor|not],k,...)
  decr = chkargs_wrap(decr, 1), -- (k) -> new
  decrby = decrby, -- (k,n) -> new
  get = chkargs_wrap(get, 1), -- (k) -> [v|nil]
  getbit = getbit, -- (k,offset) -> b
  getrange = getrange, -- (k,start,end) -> string
  getset = chkargs_wrap(getset, 2), -- (k,v) -> [oldv|nil]
  incr = chkargs_wrap(incr, 1), -- (k) -> new
  incrby = incrby, -- (k,n) -> new
  incrbyfloat = incrbyfloat, -- (k,n) -> new
  mget = mget, -- (k1,...) -> {v1,...}
  mset = mset, -- (k1,v1,...) -> true
  msetnx = msetnx, -- (k1,v1,...) -> worked? (i.e. !existed? any k)
  set = chkargs_wrap(set, 2), -- (k,v) -> true
  setbit = setbit, -- (k,offset,b) -> old
  setnx = chkargs_wrap(setnx, 2), -- (k,v) -> worked? (i.e. !existed?)
  setrange = setrange, -- (k,offset,val) -> #new
  strlen = chkargs_wrap(strlen, 1), -- (k) -> [#v|0]
  -- hashes
  hdel = hdel, -- (k,sk1,...) -> #removed
  hexists = chkargs_wrap(hexists, 2), -- (k,sk) -> exists?
  hget = chkargs_wrap(hget,2), -- (k,sk) -> v
  hgetall = chkargs_wrap(hgetall, 1), -- (k) -> map
  hincrby = hincrby, -- (k,sk,n) -> new
  hincrbyfloat = hincrbyfloat, -- (k,sk,n) -> new
  hkeys = chkargs_wrap(hkeys, 1), -- (k) -> keys
  hlen = chkargs_wrap(hlen, 1), -- (k) -> [#sk|0]
  hmget = hmget, -- (k,{sk1,...}) -> {v1,...}
  hmset = hmset, -- (k,{sk1=v1,...}) -> true
  hset = chkargs_wrap(hset, 3), -- (k,sk1,v1) -> !existed?
  hsetnx = chkargs_wrap(hsetnx, 3), -- (k,sk1,v1) -> worked? (i.e. !existed?)
  hvals = chkargs_wrap(hvals, 1), -- (k) -> values
  -- lists
  blpop = blpop, -- (k1,...) -> k,v
  brpop = brpop, -- (k1,...) -> k,v
  brpoplpush = brpoplpush, -- (k1,k2,timeout) -> v
  lindex = lindex, -- (k,i) -> v
  linsert = chkargs_wrap(linsert, 4), -- (k,mode,pivot,v) -> #list (after)
  llen = chkargs_wrap(llen, 1), -- (k) -> #list
  lpop = chkargs_wrap(lpop, 1), -- (k) -> v
  lpush = lpush, -- (k,v1,...) -> #list (after)
  lpushx = chkargs_wrap(lpushx, 2), -- (k,v) -> #list (after)
  lrange = lrange, -- (k,start,stop) -> list
  lrem = lrem, -- (k,count,v) -> #removed
  lset = lset, -- (k,i,v) -> true
  ltrim = ltrim, -- (k,start,stop) -> true
  rpop = chkargs_wrap(rpop, 1), -- (k) -> v
  rpoplpush = chkargs_wrap(rpoplpush, 2), -- (k1,k2) -> v
  rpush = rpush, -- (k,v1,...) -> #list (after)
  rpushx = chkargs_wrap(rpushx, 2), -- (k,v) -> #list (after)
  -- sets
  sadd = sadd, -- (k,v1,...) -> #added
  scard = chkargs_wrap(scard, 1), -- (k) -> [n|0]
  sdiff = sdiff, -- (k1,...) -> set (of elements in k1 & not in any of ...)
  sdiffstore = sdiffstore, -- (k0,k1,...) -> #set at k0
  sinter = sinter, -- (k1,...) -> set
  sinterstore = sinterstore, -- (k0,k1,...) -> #set at k0
  sismember = chkargs_wrap(sismember, 2), -- (k,v) -> member?
  smembers = chkargs_wrap(smembers, 1), -- (k) -> set
  smove = chkargs_wrap(smove, 3), -- (k1,k2,v) -> moved? (i.e. !member? k1)
  spop = chkargs_wrap(spop, 1), -- (k) -> [v|nil]
  srandmember = srandmember, -- (k,[count]) -> v|[v1,v2,...]
  srem = srem, -- (k,v1,...) -> #removed
  sunion = sunion, -- (k1,...) -> set
  sunionstore = sunionstore, -- (k0,k1,...) -> #set at k0
  -- zsets
  zadd = zadd, -- (k,score,member,[score,member,...])
  zcard = chkargs_wrap(zcard, 1), -- (k) -> n
  zcount = chkargs_wrap(zcount, 3), -- (k,min,max) -> count
  zincrby = zincrby, -- (k,score,v) -> score
  zinterstore = zinterstore, -- (k,numkeys,k1,...,[opts]) -> card
  zrange = zrange, -- (k,start,stop,[opts]) -> depends on opts
  zrangebyscore = zrangebyscore, -- (k,min,max,[opts]) -> depends on opts
  zrank = chkargs_wrap(zrank, 2), -- (k,v) -> rank
  zrem = zrem, -- (k,v1,...) -> #removed
  zremrangebyrank = zremrangebyrank, -- (k,start,stop) -> #removed
  zremrangebyscore = chkargs_wrap(zremrangebyscore, 3), -- (k,min,max) -> #removed
  zrevrange = zrevrange, -- (k,start,stop,[opts]) -> depends on opts
  zrevrangebyscore = zrevrangebyscore, -- (k,min,max,[opts]) -> depends on opts
  zrevrank = chkargs_wrap(zrevrank, 2), -- (k,v) -> rank
  zscore = chkargs_wrap(zscore, 2), -- (k,v) -> score
  zunionstore = zunionstore, -- (k,numkeys,k1,...,[opts]) -> card
  -- connection
  echo = chkargs_wrap(echo, 1), -- (v) -> v
  ping = ping, -- () -> true
  -- server
  flushall = flushdb, -- () -> true
  flushdb = flushdb, -- () -> true
  -- debug
  dbg_zcoherence = dbg_zcoherence,
}

local new = function()
  local r = {data = {}}
  return setmetatable(r,{__index = methods})
end

return {
  new = new,
}
