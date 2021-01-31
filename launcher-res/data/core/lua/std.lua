-- (cpp)std::vector
--[[
vector = {}
vector.__index = vector;
 
function vector.new()
	local ret = {}
	setmetatable(ret, vector)
	ret.tempVec = {}
	return ret
end
 
function vector:at(index)
	return self.tempVec[index]
end
 
function vector:push_back(val)
	table.insert(self.tempVec,val)
end
 
function vector:erase(index)
	table.remove(self.tempVec,index)
end
 
function vector:size()
    return #self.tempVec;
end
 
function vector:iter()
	local i = 0
	return function ()
        i = i + 1
        return self.tempVec[i]
    end
end
]]

-- (cpp)std::map
map = {}
map.__index = map
map.count = 0
 
function map.new()
    local ret = {}
    setmetatable(ret, map)
    ret.tempVec = {}
    return ret
end
 
function map:insert(k,v)
    if nil == self.tempVec[k] then
        self.tempVec[k] = v
        self.count = self.count + 1
    else
        self.tempVec[k] = v
    end
end
 
function map:erase(k)
    if nil ~= self.tempVec[k] then
        self.tempVec[k] = nil
        self.count = self.count - 1
    end
end
 
function map:size()
    return self.count
end
 
function map:find(k)
    return self.tempVec[k]
end
 
function map:clear()
    for k,_ in pairs(self.tempVec) do
        self.tempVec[k] = nil
    end
    self.count = 0
end
 
function map:begin()
    return next(self.tempVec, nil);
end

function map:next(key)
    return next(self.tempVec, key);
end

function map:iter()
    local i = 0
    local key = nil
    local value = nil
    return function ()
        key, value = next(self.tempVec, key);
        if key ~= nil then
            return key, value
        else
            return nil, nil
        end
    end
end