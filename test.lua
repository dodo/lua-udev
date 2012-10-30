print"require"
local udev = require 'udev'
local socket = require 'socket'
print"start"

local ud = udev() print("ud", ud, ud and ud._native)
local mon = udev.monitor(ud, "udev") print("monitor", mon, mon and mon._native, mon:getfd())

print("add subsystem devtype", mon:filter_subsystem_devtype("power_supply"))

print("start monitor", mon:start())



while true do
    if #socket.select({mon}, nil, nil) > 0 then
        local device = mon:receive()
        print("get device", device:getsyspath())
    end
end

mon:close()
ud:close()

print"done."
