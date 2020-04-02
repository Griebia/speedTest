#!/usr/bin/lua

local libspeedtest = require ("libspeedtest")
local socket =  require("socket")



locationAPISelected = 1
locationAPIs = {
    "https://ipapi.co/{ip}/json/",
    "extreme-ip-lookup.com/json/{ip}",
    "http://ip-api.com/json/{ip}",
    "https://ipgeolocation.com/{ip}",
    "https://api.ipgeolocationapi.com/geolocate/{ip}"
}

server = nil
warning = nil
error = nil
state = "START"
time = 10
silent = false

--Writes to JSON file all of the information
function writeToJSON(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    local file = io.open("/tmp/speedtest.json","w")
    file:write("{")
    if (server ~= nil) then
        file:write("\"serverURL\" : \""..server.."\",")
    end
    if tonumber(downloadSpeed) then
        file:write("\"avgDownloadSpeed\" : "..downloadSpeed..",")
    end
    if tonumber(currentDownloaded) then
        file:write("\"downloaded\" : "..currentDownloaded..",")
    end
    if tonumber(uploadSpeed) then
        file:write("\"avgUploadSpeed\" : "..uploadSpeed..",")
    end
    if tonumber(uploadSpeed) then
        file:write("\"uploaded\" : "..uploadSpeed..",")
    end
    if(error ~= nil) then
        state = "ERROR"
        file:write("\"error\" : \""..error.."\",")
    end
    if(warning ~= nil) then
        file:write("\"warning\" : \""..warning.."\",")
    end
    if(state ~= nil) then
        file:write("\"state\" : \""..state.."\"")
    end
    file:write("}")
    file:close()
end

writeToJSON(nil,nil,nil,nil)
-- Gets the current ip addresses latitude and longitude.
function getCurrentLocation()
    -- Getting the json file form https://api.myip.com with IP.
    local body = libspeedtest.getbody("https://api.myip.com")
    if body == nil or body == "" then
        warning = "Cound not determine your ip address, the server will be selected closest to 0, 0"
        return 0, 0
    end
    ip = parseIP(body)
    body = libspeedtest.getbody(string.gsub(locationAPIs[locationAPISelected],"{ip}",ip))
    if (string.match(body,"lat") or string.match(body, "coords")) then
        -- Gets from the retuned data the latitude and longitude.
        return parseLocation(body)
    end

    if(#locationAPIs > (locationAPISelected)) then
        locationAPISelected = locationAPISelected + 1
        return getCurrentLocation()
    end

    return false;
end

-- Returns IP address from th given json
function parseIP(body)
    local i, j = string.find(body ,"ip")
    i = j + 3
    j = string.find(body,"\"",i+1)
    ip = string.sub(body,i+1,j-1)
    return ip
end

-- Parse the latitude and longitude form given string.
function parseLocation(body)
    local latitude
    local longitude
    if string.match(body,"coords") then
        local i,j = string.find(body, "coords")
        local _,_, lat1, lat2, lon1, lon2 = string.find(body,"(%-?%d+).(%d+),(%-?%d+).(%d+)",j)
        latitude = lat1.."."..lat2
        longitude = lon1.."."..lon2
    else
        local i,j = string.find(body, "lat")
        local _,_,  integer, decimal = string.find(body,"(%-?%d+).(%d+)",j)
        latitude = integer.."."..decimal
        i,j = string.find(body, "lon")
        _,_,  integer, decimal = string.find(body,"(%-?%d+).(%d+)",j)
        longitude = integer.."."..decimal 
    end
    return tonumber(latitude), tonumber(longitude)
end


function parseURL(body)
    local i,j = string.find(body,"url=\"")
    local i = string.find(body,"\"",j+1)
    return string.sub(body, j+1, i-1)
end

--Trims the link by removing https:// or http:// and removing everything from the link that goes from / character.
function trimLink(link)
    local cutlink = link
    if(string.match(link,"//")) then 
        local i, j = string.find(link, "//")
        i = string.find(link, "\n")
        cutLink = string.sub(link, j+1, i)
    end
    if(string.match(cutlink,":")) then
        i = string.find(cutLink, ":")
        cutLink = string.sub(cutLink, 0, i-1)
    end
    return cutLink
end

-- Calculates the distances between 2 positions.
function calculateDistance(lat1,lon1,lat2,lon2)
    local R = 6371e3
    local fi1 = math.rad(lat1)
    local fi2 = math.rad(lat2)
    local deltaLam = math.rad((lon2 - lon1))
    local d = math.acos( math.sin(fi1) * math.sin(fi2) + math.cos(fi1) * math.cos(fi2) * math.cos(deltaLam)) * R
    return d
end

-- Changes bytes to other numbers.
function convertBytes(bytes,mbps)
    if mbps then
        if bytes > 1000000000 then 
            return string.format("%.3f",(bytes/125000000)).."gbps"
        elseif bytes > 1000000 then
            return string.format("%.3f",(bytes/125000)).."mbps"
        elseif bytes > 1000 then
            return string.format("%.3f",(bytes/125)).."kbps"
        end
        return bytes.."bps"
    else
        if bytes > (1024*1024*1024) then 
            return string.format("%.3f",(bytes/(1024*1024*1024))).."GB"
        elseif bytes > (1024*1024) then
            return string.format("%.3f",bytes/(1024*1024)).."MB"
        elseif bytes > 1024 then
            return string.format("%.3f",(bytes/1024)).."KB"
        end
        return bytes.."B"
    end
end

count = 0
dowloadBytes = 0
uploadBytes = 0

--Writes by choosen paramters to specific places
function writeData(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    if(currentDownloaded == downloadBytes and currentUpload == uploadBytes) then
        count = count + 1
        if count == 3 then
            error = "Connection to the speed test server was lost."
            writeData(nil,nil,nil,nil)
            os.exit()
        end
    else
        count = 0;
    end

    downloadBytes = currentDownloaded
    uploadBytes = currentUpload

    if silent then
        writeToJSON(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    else
        writeToConsole(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    end
    if(warning ~= nil) then
        warning = nil
    end
    if(error ~= nil) then
        error = nil
    end
end

function close()
    print("Closing the speed test")
end

--Writes to console and JSON file
function writeToConsole (downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    if(warning ~= nil) then
        print("Warning: "..warning)
    end
    if(error ~= nil) then
        print("Error: "..error)
    end
    if tonumber(currentDownloaded) then
        if currentDownloaded > 0 then
            print("Average download speed is "..convertBytes(downloadSpeed,true).." Current downloaded "..convertBytes(currentDownloaded, false))
        else
            print("Average upload speed is "..convertBytes(uploadSpeed,true).." Current uploaded "..convertBytes(currentUpload, false)) 
        end 
    end
    writeToJSON(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    return true
end

--Acts by the given flag from avg
function flagCheck(num,flag)
    local tmp = 0;
    if flag == "--help" then
        print("usage: speedtest [options]\nAvailible options are:\n--help      shows usage of file\n-s          set silent mode\n-u [url]    set server\n-t [time]    set test time\n")
	os.exit()
    elseif flag == "-s" then 
        silent = true;
    elseif flag == "-u" then
        server = arg[num+1]
        if server == nil then
            warning = "The link was not set correctly"
            writeData(nil,nil,nil,nil)
        end
        server = trimLink(server)
        if socket.connect(server,80) == nil then
            error = "There was no connection to the given server"
            writeData(nil,nil,nil,nil)
        end
        tmp = 1
    elseif flag == "-t" then
        if arg[num+1] ~= nil then
            time = arg[num+1]
            tmp = 1
        else
            warning = "The time was not set correctly"
            writeData(nil,nil,nil,nil)
        end
    elseif flag == "-t" then
        if arg[num+1] ~= nil then
            time = arg[num+1]
            tmp = 1
        else
            warning = "The time was not set correctly"
            writeData(nil,nil,nil,nil)
        end
    else
        print("The is no such option as "..flag)
    end
    return tmp
end

function getServerList()
    local body
    local file = io.open("/tmp/serverlist.xml","r")
    if (file ~= nil) then
        body = file:read("*all")
        if body == nil or body == "" then
            os.remove("/tmp/serverlist.xml")
            error = "Could not get the server list."
            writeData(nil,nil,nil,nil)
            os.exit()
        end
        file:close()
    else
        file = io.open("/tmp/serverlist.xml","w")
        body = libspeedtest.getbody("https://c.speedtest.net/speedtest-servers-static.php")
        if body == nil or body == "" then
            error = "Could not get the server list."
            writeData(nil,nil,nil,nil)
            os.exit()
        end
        file:write(body)
        file:close()
    end

    return body
end

function getClosestServer()
    body = getServerList()
    local lat1, lon1 = getCurrentLocation()
    local i = 1
    local server
    local minDistance = -1
    for line in readLines(body) do
        if string.match(line,"lat") then
            lat2, lon2 = parseLocation(line)
            if(lat2 == nil or lon2 == nil )then
                return
            end
            local dist = calculateDistance(lat1,lon1,lat2,lon2)
            if(minDistance > dist or minDistance == -1) then
                minDistance = dist
                server = parseURL(line)
            end
        end
    end
    return trimLink(server);
end

function cheakConnection(url)
    local connection = socket.tcp()
    connection:settimeout(1000)
    local result = connection:connect(url, 80)
    connection:close()
    if result then
        return true
    end
    return false
end

-- Reads line by line a string.
function readLines(s)
    if s:sub(-1)~="\n" then s=s.."\n" end
    return s:gmatch("(.-)\n")
end

--Checks if there is a any added flags and does them accordingly.
if(#arg > 0) then
    local i = 1
    while #arg >= i do
        local tmp = flagCheck(i,arg[i])
        i = i + tmp
        i = i + 1
    end
end

--Looks for internet connection
state = "CHECKING_CONNECTION"
writeData(nil, nil, nil, nil)
if not cheakConnection("www.google.com") then
    error = "Internet connection is required to use this application."
    writeData(nil, nil, nil, nil)
    os.exit()
end

if not silent then
    print("This speedtest can use up a lot of internet data. Do you want to continue?(Y/N)")
    local info = io.read();
    if not (info == "y") and not (info == "Y") then
        os.exit()
    end
end


if not server then
    if not silent then
        print("Finding closest server..")
    end
    state = "FINDING_SERVER"
    writeToJSON(nil,nil,nil,nil)
    server = getClosestServer();
    if(not server) then
        error = "We could not determine the closes server to you."
        writeData(nil,nil,nil,nil)
        os.exit()
    end
end

if not cheakConnection(server) then
    error = "There were no response from the selected server."
    writeData(nil, nil, nil, nil)
    os.exit()
end

if not silent then
    print("The server that is selected: "..server)
end

state = "TESTING_DOWNLOAD"

isError, res = libspeedtest.testspeed(server..":8080/download", time, false)
if isError then
    error = res
    writeData(nil,nil,nil,nil)
    os.exit()
end 

state = "COOLDOWN"
writeToJSON(nil,nil,nil,nil)
socket.sleep(3);
state = "TESTING_UPLOAD"

isError, res = libspeedtest.testspeed(server..":8080/speedtest/upload.php", time, true)
if isError then
    error = res
    writeData(nil,nil,nil,nil)
    os.exit()
end

state = "FINISHED"
writeToJSON(0,0,0,0)
os.remove("/var/run/speedtest.pid")
--gcc luaWrapper.c -shared -o libspeedtest.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
--gcc speedtest.c -shared -o libspeedtest.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
--/usr/lib/lua/luci/  
