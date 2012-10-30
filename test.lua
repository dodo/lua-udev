print"require"
local udev = require 'udev'
local socket = require 'socket'
print"start"

local ud = udev() print("ud", ud, ud and ud._native)
local mon = udev.monitor(ud, "udev") print("monitor", mon, mon and mon._native, mon:getfd())

for _, subsystem in ipairs({"power_supply","drm","net"}) do
    print("add subsystem("..subsystem..") devtype",
          mon:filter_subsystem_devtype(subsystem))
end

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
