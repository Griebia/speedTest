module("luci.controller.speed_test", package.seeall)

function index()
	entry({"admin", "services", "speed_test"}, template("speed_test"),_("Speed test"), 100)
	entry({"admin", "services", "speed_test", "getJSON"}, call("getJSON"))
	entry({"admin", "services", "speed_test", "start"}, call("startSpeedTest"))
	entry({"admin", "services", "speed_test", "getServers"}, call("getServers"))
	entry({"admin", "services", "speed_test", "checkInternetConnection"}, call("checkInternetConnection")) 
end

function checkInternetConnection()
	local socket = require("socket");
	local connection = socket.tcp()
    connection:settimeout(1000)
    local result = connection:connect("google.com", 80)
    connection:close()
    if result then
		luci.http.write("true")
		return
    end
    luci.http.write("false")
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
	call = "speedtest -s"
	if(url ~= nil) then
		call = call.." -u "..url
	end
	luci.util.perror("Started speedtest")
	luci.sys.call(call)
end

function getServers()
	local body
    local file = io.open("/tmp/serverlist.xml","r")
    if (file ~= nil) then
        body = file:read("*all")
        if body == nil or body == "" then
			os.remove("/tmp/serverlist.xml")
			luci.util.perror("The serverlist was not found")
			luci.http.write("Error");
			return
        end
        file:close()
    else
		file = io.open("/tmp/serverlist.xml","w")
		local libspeedtest = require("libspeedtest")
        body = libspeedtest.getbody("https://c.speedtest.net/speedtest-servers-static.php")
        if body == nil or body == "" then
			luci.util.perror("The serverlist was not found")
			luci.http.write("Error");
			return
        end
        file:write(body)
        file:close()
	end
	luci.http.prepare_content("application/xml")
	luci.http.write(body)
end