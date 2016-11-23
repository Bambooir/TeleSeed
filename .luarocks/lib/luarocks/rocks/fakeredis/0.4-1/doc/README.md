# fakeredis

[![Build Status](https://travis-ci.org/catwell/fakeredis.png?branch=master)](https://travis-ci.org/catwell/fakeredis)

## Presentation

fakeredis is a Redis mock for Lua with the same interface as [redis-lua](https://github.com/nrk/redis-lua).

## Dependencies

Lua 5.1 users will need [Lua BitOp](http://bitop.luajit.org/).

If you do not set a custom sleep function and LuaSocket is not installed,
a busy wait will be used for `sleep()`.

Tests require [cwtest](https://github.com/catwell/cwtest).

## Usage

```lua
local R = fakeredis.new()
R:set("foo", "bar")
local foo = R:get("bar")
```

See [redis-lua](https://github.com/nrk/redis-lua) for more.

## Design

fakeredis tries to keep its interface compatible with redis-lua but is not fully complete, see supported.txt for supported commands.

fakeredis does not support multiple DBs. You should probably not use them anyway. If you really need SELECT in tests it is easy to write a small wrapper that uses multiple instances of fakeredis.

## Copyright

Copyright (c) 2012-2015 Pierre Chapuis
