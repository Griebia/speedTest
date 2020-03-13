module("luci.controller.speed_test", package.seeall)

function index()
	entry({"admin", "services", "speed_test"}, template("speed_test"),_("Speed test"), 100)
	entry({"admin", "services", "speed_test", "getJSON"}, call("getJSON"))
	entry({"admin", "services", "speed_test", "start"}, call("startSpeedTest"))
	entry({"admin", "services", "speed_test", "getServers"}, call("getServers")) 
end

function getJSON()
	local f = assert(io.open("/tmp/speedtest.json", "rb"))
    local response = f:read("*all")
    f:close()
	luci.http.prepare_content("application/json")
	luci.http.write(response)
end

function startSpeedTest()
	url = luci.http.formvalue("url")
	call = "/usr/lib/lua/luci/speedtest.lua -s"
	if(url ~= nil) then
		call = call.." -u "..url
	end
	luci.util.perror("Started speedtest")
	luci.sys.call(call)
end

function getServers()
	local f = io.open("/tmp/serverlist.xml", "rb")
	if f == nil then
		luci.util.perror("The serverlist was not found")
		local libspeedtest = require("libspeedtest")
		local body = libspeedtest.getbody("https://c.speedtest.net/speedtest-servers-static.php")
		f = io.open("/tmp/serverlist.xml", "w+")
		f:write(body)
		f:close()
		f = io.open("/tmp/serverlist.xml", "rb")
	end
    local response = f:read("*all")
    f:close()
	luci.http.prepare_content("application/xml")
	luci.http.write(response)
end