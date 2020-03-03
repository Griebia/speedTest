#!/usr/bin/lua

local libspeedtest = require ("libspeedtest")
local socket =  require("socket")

locationAPISelected = 1
locationAPIs = {
    "a",
    "b",
    "c",
    "https://ipapi.co/{ip}/json/",
    "extreme-ip-lookup.com/json/{ip}",
    "http://ip-api.com/json/{ip}",
    "https://ipgeolocation.com/{ip}",
    "https://api.ipgeolocationapi.com/geolocate/{ip}"
}

server = nil
warning = nil
error = nil
time = 3
silent = false

-- Gets the current ip address latitude and longitude.
function getCurrentLocation()
    -- Getting the json file form https://api.myip.com with IP.
    local body = libspeedtest.getbody("https://api.myip.com")
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

-- Gets the links latitude and longitude.
function getLocation(link)
    local trimedLink = trimLink(link)
    -- Get the ip address of the site
    local client = socket.connect( trimedLink, 80 )
    if(client == nil) then
        warning = "To the url "..link.."we couldn't connect"
        writeData(nil,nil,nil,nil)
        return false
    end
    local ip, port = client:getpeername()
    -- Gets the ip address information.
    local body = libspeedtest.getbody(string.gsub(locationAPIs[locationAPISelected],"{ip}",ip))
    if (string.match(body,"lat") or string.match(body, "coords")) then
        -- Gets from the retuned data the latitude and longitude.
        return parseLocation(body)
    end

    if(#locationAPIs > (locationAPISelected)) then
        locationAPISelected = locationAPISelected + 1
        return getLocation(link)
    end
    return false;
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
    if(string.match(cutlink,"/")) then
        i = string.find(cutLink, "/")
        cutLink = string.sub(cutLink, 0, i-1)
    end
    return cutLink
end

-- Finds the closest server from the current location.
function findClosestServer(servers)
    local server;
    local lat1, lon1 = getCurrentLocation()
    local count = #servers
    local minDistance = 999999999999
    for i=1,count,1 do 
        local lat2, lon2 = getLocation(servers[i]);
        if(lat2 == nil or lon2 == nil )then
            return
        end
        local dist = calculateDistance(lat1,lon1,lat2,lon2)
        if(minDistance > dist) then
            minDistance = dist
            server = servers[i]
        end
    end
   return server
end



function findClosestServer(url, latURL, lonURL)
    local server;
    local lat1, lon1 = getCurrentLocation()
    local count = #servers
    local minDistance = 999999999999
    for i=1,count,1 do 
        local lat2, lon2 = getLocation(servers[i]);
        if(lat2 == nil or lon2 == nil )then
            return
        end
        local dist = calculateDistance(lat1,lon1,lat2,lon2)
        if(minDistance > dist) then
            minDistance = dist
            server = servers[i]
        end
    end
   return server
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

--Writes by choosen paramters to specific places
function writeData(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
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

--Writes to JSON file all of the information
function writeToJSON(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    local file = io.open("/usr/lib/lua/luci/speedtest.json","w")
    file:write("{")
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
        file:write("\"uploaded\" : "..uploadSpeed.."")
    end
    if(error ~= nil) then
        file:write("\"error\" : \""..error.."\",")
    end
    if(warning ~= nil) then
        file:write("\"warning\" : \""..warning.."\",")
    end
    file:write("}")
    file:close()
end

--Acts by the given flag from avg
function flagCheck(num,flag)
    local tmp = 0;
    if flag == "--help" then
        print("usage: speedtest [options]\nAvailible options are:\n--help      shows usage of file\n-s          set silent mode\n-u [url]    set server\n-t [time]    set test time\n")
    elseif flag == "-s" then 
        silent = true;
    elseif flag == "-u" then
        server = arg[num+1]
        if server == nil then
            warning = "The link was not set correctly"
            writeData(nil,nil,nil,nil)
        end
        if socket.connect(server,80) == nil then
            warning = "There was no connection to the given URL"
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

function getClosestServer()
    body = libspeedtest.getbody("https://c.speedtest.net/speedtest-servers-static.php")
    local lat1, lon1 = getCurrentLocation()
    local i = 1
    local server
    local minDistance = -1
    for line in magiclines(body) do
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

function magiclines(s)
    if s:sub(-1)~="\n" then s=s.."\n" end
    return s:gmatch("(.-)\n")
end
--Checks if there is a any added flags and does the accordingly 
if(#arg > 0) then
    local i = 1
    while #arg >= i do
        local tmp = flagCheck(i,arg[i])
        i = i + tmp
        i = i + 1
    end
end

--Looks for internet connection
if socket.connect("www.google.com",80) == nil then
    error = "An internet connection is required to use this application."
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
    server = getClosestServer();
end

if(not server) then
    error = "We could not determine the closes server to you."
    writeData(nil,nil,nil,nil)
    os.exit()
end

if not silent then
    print("The server that is selected: "..server)
end

isError, res = libspeedtest.testspeed(server.."/download", time, false)
if isError then
    error = res
    writeData(nil,nil,nil,nil)
    os.exit()
end 
--https://de.testmy.net/uploader
--http://speed-kaunas.telia.lt:8080/speedtest/upload.php

-- getServerData()
print("Upload")
isError, res = libspeedtest.testspeed(server.."/speedtest/upload.php", 200, true)
if isError then
    error = res
    writeData(nil,nil,nil,nil)
    os.exit()
end

--gcc luaWrapper.c -shared -o libspeedtest.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
--gcc speedtest.c -shared -o libspeedtest.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
--/usr/lib/lua/luci/