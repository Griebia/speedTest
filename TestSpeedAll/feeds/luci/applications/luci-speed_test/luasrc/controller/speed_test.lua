module("luci.controller.speed_test", package.seeall)

function index()
	entry({"admin", "services", "speed_test"}, template("speed_test"),_("Speed test APP"), 100)
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
	execSpeed = luci.sys.exec("ps | grep [s]peedtest")

	if(execSpeed == "") then
		if(url ~= nil) then
			call = call.." -u "..url
		end
		--luci.http.write(call)
		luci.sys.call(call)	
		luci.util.perror("Started speedtest")
	end
end

function getServers()
	local f = assert(io.open("/tmp/serverlist.xml", "rb"))
    local response = f:read("*all")
    f:close()
	luci.http.prepare_content("application/xml")
	luci.http.write(response)
end