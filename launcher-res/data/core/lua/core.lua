-- Note: This file is loaded automatically by the engine.

--[========[Config Manipulation Functions]========]

wml = {}
wml.tovconfig = rose.tovconfig
wml.tostring = rose.debug

local function ensure_config(cfg)
	if type(cfg) == 'table' then
		return true
	end
	if type(cfg) == 'userdata' then
		if getmetatable(cfg) == 'wml object' then return true end
		error("Expected a table or wml object but got " .. getmetatable(cfg), 3)
	else
		error("Expected a table or wml object but got " .. type(cfg), 3)
	end
	return false
end

--! Returns the first subtag of @a cfg with the given @a name.
--! If @a id is not nil, the "id" attribute of the subtag has to match too.
--! The function also returns the index of the subtag in the array.
function wml.get_child(cfg, name, id)
	ensure_config(cfg)
	for i,v in ipairs(cfg) do
		if v[1] == name then
			local w = v[2]
			if not id or w.id == id then return w, i end
		end
	end
end

--! Returns the nth subtag of @a cfg with the given @a name.
--! (Indices start at 1, as always with Lua.)
--! The function also returns the index of the subtag in the array.
function wml.get_nth_child(cfg, name, n)
	ensure_config(cfg)
	for i,v in ipairs(cfg) do
		if v[1] == name then
			n = n - 1
			if n == 0 then return v[2], i end
		end
	end
end

--! Returns the number of subtags of @a with the given @a name.
function wml.child_count(cfg, name)
	ensure_config(cfg)
	local n = 0
	for i,v in ipairs(cfg) do
		if v[1] == name then
			n = n + 1
		end
	end
	return n
end

--! Returns an iterator over all the subtags of @a cfg with the given @a name.
function wml.child_range(cfg, tag)
	ensure_config(cfg)
	local iter, state, i = ipairs(cfg)
	local function f(s)
		local c
		repeat
			i,c = iter(s,i)
			if not c then return end
		until c[1] == tag
		return c[2]
	end
	return f, state
end

--! Returns an array from the subtags of @a cfg with the given @a name
function wml.child_array(cfg, tag)
	ensure_config(cfg)
	local result = {}
	for val in wml.child_range(cfg, tag) do
		table.insert(result, val)
	end
	return result
end

--! Removes the first matching child tag from @a cfg
function wml.remove_child(cfg, tag)
	ensure_config(cfg)
	for i,v in ipairs(cfg) do
		if v[1] == tag then
			table.remove(cfg, i)
			return
		end
	end
end

--! Removes all matching child tags from @a cfg
function wml.remove_children(cfg, ...)
	for i = #cfg, 1, -1 do
		for _,v in ipairs(...) do
			if cfg[i] == v then
				table.remove(cfg, i)
			end
		end
	end
end

--[========[WML Tag Creation Table]========]

local create_tag_mt = {
	__metatable = "WML tag builder",
	__index = function(self, n)
		return function(cfg) return { n, cfg } end
	end
}

wml.tag = setmetatable({}, create_tag_mt)

--[========[Config / Vconfig Unified Handling]========]

function wml.literal(cfg)
	if type(cfg) == "userdata" then
		return cfg.__literal
	else
		return cfg or {}
	end
end

function wml.parsed(cfg)
	if type(cfg) == "userdata" then
		return cfg.__parsed
	else
		return cfg or {}
	end
end

function wml.shallow_literal(cfg)
	if type(cfg) == "userdata" then
		return cfg.__shallow_literal
	else
		return cfg or {}
	end
end

function wml.shallow_parsed(cfg)
	if type(cfg) == "userdata" then
		return cfg.__shallow_parsed
	else
		return cfg or {}
	end
end

--[========[Deprecation Helpers]========]

-- Need a textdomain for translatable deprecation strings.
local _ = rose.textdomain "rose"

-- Marks a function or subtable as deprecated.
-- Parameters:
---- elem_name: the full name of the element being deprecated (including the module)
---- replacement: the name of the element that will replace it (including the module)
---- level: deprecation level (1-4)
---- version: the version at which the element may be removed (level 2 or 3 only)
---- Set to nil if deprecation level is 1 or 4
---- elem: The actual element being deprecated
---- detail_msg: An optional message to add to the deprecation message
function rose.deprecate_api(elem_name, replacement, level, version, elem, detail_msg)
	local message = detail_msg or ''
	if replacement then
		message = message .. " " .. rose.format(
			_"(Note: You should use $replacement instead in new code)",
			{replacement = replacement})
	end
	if type(level) ~= "number" or level < 1 or level > 4 then
		local err_params = {level = level}
		-- Note: This message is duplicated in src/deprecation.cpp
		-- Any changes should be mirrorred there.
		error(rose.format(_"Invalid deprecation level $level (should be 1-4)", err_params))
	end
	local msg_shown = false
	if type(elem) == "function" then
		return function(...)
			if not msg_shown then
				msg_shown = true
				rose.deprecated_message(elem_name, level, version, message)
			end
			return elem(...)
		end
	elseif type(elem) == "table" then
		-- Don't clobber the old metatable.
		local old_mt = getmetatable(elem) or {}
		local mt = {}
		for k,v in pairs(old_mt) do
			mt[k] = old_mt[v]
		end
		mt.__index = function(self, key)
			if not msg_shown then
				msg_shown = true
				rose.deprecated_message(elem_name, level, version, message)
			end
			if type(old_mt) == "table" then
				if type(old_mt.__index) == 'function' then
					return old_mt.__index(self, key)
				elseif type(old_mt.__index) == 'table' then
					return old_mt.__index[key]
				else
					-- As of 2019, __index must be either a function or a table. If you ever run into this error,
					-- add an elseif branch for wrapping old_mt.__index appropriately. 
					wml.error('The wrapped __index metamethod of a deprecated object is neither a function nor a table')
				end
			end
			return elem[key]
		end
		mt.__newindex = function(self, key, val)
			if not msg_shown then
				msg_shown = true
				rose.deprecated_message(elem_name, level, version, message)
			end
			if type(old_mt) == "table" and old_mt.__newindex ~= nil then
				return old_mt.__newindex(self, key, val)
			end
			elem[key] = val
		end
		return setmetatable({}, mt)
	else
		rose.log('warn', "Attempted to deprecate something that is not a table or function: " ..
			elem_name .. " -> " .. replacement .. ", where " .. elem_name .. " = " .. tostring(elem))
	end
	return elem
end

-- rose.tovconfig = rose.deprecate_api('rose.tovconfig', 'wml.tovconfig', 1, nil, rose.tovconfig)
-- rose.debug = rose.deprecate_api('rose.debug', 'wml.tostring', 1, nil, rose.debug)
