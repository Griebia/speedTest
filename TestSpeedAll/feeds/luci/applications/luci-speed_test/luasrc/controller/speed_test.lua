module("luci.controller.speed_test", package.seeall)

function index()
	entry({"admin", "services", "speed_test"}, template("speed_test"),_("Speed test APP"), 100)
	entry({"admin", "services", "speed_test", "getJSON"}, call("getJSON"))
	entry({"admin", "services", "speed_test", "start"}, call("startSpeedTest"))
end

function getJSON()
	local f = assert(io.open("/usr/lib/lua/luci/speedtest.json", "rb"))
    local response = f:read("*all")
    f:close()
	luci.http.prepare_content("application/json")
	luci.http.write(response)
end

function startSpeedTest()
	luci.sys.call("/usr/lib/lua/luci/speedtest.lua -s")
end
