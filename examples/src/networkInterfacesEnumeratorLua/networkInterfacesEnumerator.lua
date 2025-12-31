#!/usr/bin/env lua

-- Network Interfaces Enumerator Lua Example

-- Load the networkInterfaceHelper module
local nih = require("la_networkInterfaceHelper")
print("Using " .. nih.getLibraryName() .. " v" .. nih.getLibraryVersion())
print(nih.getLibraryCopyright())

function displayInterfaces()
	print("Available interfaces:")

	local intNum = 1
	local obs = nih.la.networkInterface.NetworkInterfaceHelper.DefaultedObserver()
	swig_override(obs, "onInterfaceAdded", function(self, intfc)
		print(intNum .. ": " .. intfc.id)
		print("  Description:  " .. intfc.description)
		print("  Alias:        " .. intfc.alias)
		print("  MacAddress:   " .. nih.la.networkInterface.NetworkInterfaceHelper.macAddressToString(intfc.macAddress))
		print("  Type:         " .. intfc.type)
		print("  Enabled:      " .. (intfc.isEnabled and "YES" or "NO"))
		print("  Connected:    " .. (intfc.isConnected and "YES" or "NO"))
		print("  Virtual:      " .. (intfc.isVirtual and "YES" or "NO"))

		if intfc.ipAddressInfos:size() > 0 then
			print("  IP Addresses:")
			for i = 0, intfc.ipAddressInfos:size() - 1 do
				local info = intfc.ipAddressInfos[i]
				if info.address:getType() == nih.la.networkInterface.IPAddress.Type_V4 then
					print("    " ..
						info.address:toString() ..
						" (" ..
						info.netmask:toString() ..
						") -> " .. info:getNetworkBaseAddress():toString() .. " / " .. info:getBroadcastAddress():toString())
				else
					-- IPv6
					print("    " ..
						info.address:toString() ..
						"/" ..
						nih.la.networkInterface.IPAddress.prefixLengthFromPackedV6(info.netmask:getIPV6Packed()) ..
						" -> " .. info:getNetworkBaseAddress():toString())
				end
			end
		end

		if intfc.gateways:size() > 0 then
			print("  Gateways:")
			for i = 0, intfc.gateways:size() - 1 do
				local ip = intfc.gateways[i]
				print("    " .. ip:toString())
			end
		end

		intNum = intNum + 1
	end)

	local helper = nih.la.networkInterface.NetworkInterfaceHelper.getInstance()
	helper:registerObserver(obs)
	helper:unregisterObserver(obs)
end

displayInterfaces()
