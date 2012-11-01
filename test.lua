print"require"
local udev = require 'udev'
local socket = require 'socket'
print"start"

local ud = udev() print("ud", ud, ud and ud._native)

local enum = udev.enumerate(ud)
assert(enum:match_subsystem("power_supply"))
enum:match_property("power_supply_name", "AC")
print"scan subsystems"
assert(enum:scan_subsystems()) for k,v in pairs(enum:getlist()) do print(k,v) end
print"scan devices"
assert(enum:scan_devices())    for k,v in pairs(enum:getlist()) do print(k,v) end
enum:close()

local mon = udev.monitor(ud, "udev") print("monitor", mon, mon and mon._native, mon:getfd())

    print("add subsystem devtype", mon:filter_subsystem_devtype())

print("start monitor", mon:start())



while true do
    if #socket.select({mon}, nil, nil) > 0 then
        local device = mon:receive()
        print("["..tostring(device:getseqnum()).."] "..device:getaction().." device", device:getsyspath())
        print("properties:")
        for k,v in pairs(device:getproperties()) do
            print("", k, v)
        end
        print("sysattrs:")
        for k,v in pairs(device:getsysattrs()) do
            print("", k, v)
        end
        device:close()
    end
end

mon:close()
ud:close()

print"done."
