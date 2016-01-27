--print('MQTTDevice')

-- Log only devices not sensors
tc = next(devicechanged)
mydevice = tostring(tc)
if (mydevice:sub(1,1) ~= "s") then
	for i, v in pairs(devicechanged) do
		if (v == 'Set Level') then
			w = otherdevices_svalues[i]
		else
			w = ''
		end
		print("***** " .. i .. " " .. v .. " " .. w)
	end
end

-- MQTT devices and sensors
--       0  1  2   3   4   5   6   7   8   9   10  11  12  13  14  15   16
mydim = {0, 6, 13, 20, 26, 33, 40, 46, 53, 60, 66, 73, 80, 86, 93, 100, 100}

tc = next(devicechanged)
mydevice = tostring(tc)
for i, v in pairs(devicechanged) do
	if (v == 'Set Level') then
	  myd = tonumber(otherdevices_svalues[i] +1)
		if (myd > 16) then
			myd = 16
		end
		w = ": " .. mydim[myd] .. " %"
	else
		w = ""
	end
--	print("+++++ " .. i .. " " .. v .. " " .. w)

	if ((i:sub(1,3) ~= "sid") and (v ~= '')) then

		k = "cmnd"
		if (i:sub(1,1) == "s") then  -- Remove sensor indication character (s)
		  i = i:sub(2)
		  k = "stat"
		end

		if (tonumber(v)) then
		  myv = tonumber(string.format("%.1f",v))
	  else
			myv = v
	  end
		mycmnd = "domoticz-lua-mqtt '" .. i .. "' '" .. myv .. w .. "'"
		print("----- " .. mycmnd .. " " .. k)
		os.execute(mycmnd)

	end
end

commandArray = {}

return commandArray
