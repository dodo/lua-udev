print"require"
local udev = require 'udev'
print"start"

local ud = udev() print("ud", ud, ud and ud._native)
local mon = udev.monitor(ud, "udev") print("monitor", mon, mon and mon._native, mon:getfd())

print("add subsystem devtype", mon:filter_subsystem_devtype("power_supply"))

print("start monitor", mon:start())



while true do
    mon:receive()
end

mon:close()
ud:close()

print"done."
