-- Note: This file is loaded automatically by the engine.

-- user to Table Safe Access. for exmple: (table or E).field
E = {};
nposm = -1;
renderer = 0x8a7b6c5d4e3f29; -- integer value

local mt = {
	__index = function(self, k)
		if k ~= "__tostring" then
			error("Tried to access an empty package", 2)
		end
	end,
	__newindex = function()
		error("Tried to access an empty package", 2)
	end,
	__metatable = "empty package",
	__tostring = function() return "{empty package}" end,
}
local empty_pkg = setmetatable({}, mt)

local function resolve_package(pkg_name)
	-- pkg_name must be format: lua/<filename>.lua
	if pkg_name[#pkg_name] == '/' then
		pkg_name = pkg_name:sub(1, -2)
	end
	if rose.have_file(pkg_name) then 
		return pkg_name 
	end
	return nil
end

-- TODO: Currently if you require a file by different (relative) paths, each will be a different copy.
function rose.require(pkg_name)
	-- First, check if the package is already loaded
	local loaded_name = resolve_package(pkg_name)
	-- don't user cache. for applet, it's *.lua require load every time.
	-- if loaded_name and rose.package[loaded_name] then
	--	return rose.package[loaded_name]
	-- end
	if not loaded_name then
		rose.log("err", "Failed to load required package: " .. pkg_name, true)
		return nil
	end

	-- Next, if it's a single file, load the package with dofile
	if rose.have_file(loaded_name, true) then
		local pkg = rose.dofile(loaded_name)
		rose.package[loaded_name] = pkg or empty_pkg
		return pkg
	else -- If it's a directory, load all the files therein
		local files = rose.read_file(loaded_name)
		local pkg = {}
		for i = files.ndirs + 1, #files do
			if files[i]:sub(-4) == ".lua" then
				local subpkg_name = files[i]:sub(1, -5)
				pkg[subpkg_name] = rose.require(loaded_name .. '/' .. files[i])
			end
		end
		rose.package[loaded_name] = pkg
		return pkg
	end
end

function rose.lua_breakpoint()
	rose.cpp_breakpoint();
end

return empty_pkg
