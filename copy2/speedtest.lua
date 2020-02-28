#!/usr/bin/lua

local libspeedtest = require ("libspeedtest")
local socket =  require("socket")

local uploadServers = {
    "https://jp.testmy.net/uploader",
    "https://ny.testmy.net/uploader",
    "https://de.testmy.net/uploader",
    "https://sg.testmy.net/uploader",
    "https://au.testmy.net/uploader",
    "https://in.testmy.net/uploader",
    "https://lax.testmy.net/uploader",
    "https://uk.testmy.net/uploader"
}

local downloadServers = {
    "http://speedtest.tele2.net/10GB.zip",
    "http://mirror.nl.leaseweb.net/speedtest/1000mb.bin",
    "http://speedtest.as5577.net/1000mb.bin",
    "http://mirror.sfo12.us.leaseweb.net/speedtest/1000mb.bin",
    "http://mirror.wdc1.us.leaseweb.net/speedtest/1000mb.bin"
}

local downloadSelected;
local uploadSelected;

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

warning = nil
error = nil
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
function convertBytes(bytes)
    if bytes > 1000000000 then 
        return (bytes/1000000000).."gb"
    elseif bytes > 1000000 then
        return (bytes/1000000).."mb"
    elseif bytes > 1000 then
        return (bytes/1000).."kb"
    end
    return bytes.."b"
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
            print("Average download speed is "..convertBytes(downloadSpeed).."/s Current downloaded "..convertBytes(currentDownloaded))
        else
            print("Average upload speed is "..convertBytes(uploadSpeed).."/s Current uploaded "..convertBytes(currentUpload)) 
        end 
    end
    writeToJSON(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    return true
end

--Writes to JSON file all of the information
function writeToJSON(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    local file = io.open("speedtest.json","w")
    file:write("{\n")
    if tonumber(downloadSpeed) then
        file:write("\t\"avgDownlaodSpeed\":"..downloadSpeed.."\n")
    end
    if tonumber(currentDownloaded) then
        file:write("\t\"downloaded\":"..currentDownloaded.."\n")
    end
    if tonumber(uploadSpeed) then
        file:write("\t\"avgUploadSpeed\":"..uploadSpeed.."\n")
    end
    if tonumber(uploadSpeed) then
        file:write("\t\"uploaded\":"..uploadSpeed.."\n")
    end
    if(error ~= nil) then
        file:write("\t\"error\":\""..error.."\"\n")
    end
    if(warning ~= nil) then
        file:write("\t\"warning\":\""..warning.."\"\n")
    end
    file:write("}")
    file:close()
end

--Acts by the given flag from avg
function flagCheck(num,flag)
    local tmp = 0;
    if flag == "--help" then
        print("usage: speedtest [options]\nAvailible options are:\n--help      shows usage of file\n-s          set silent mode\n-d [url]    set download server\n-u [url]    set upload server\n")
    elseif flag == "-s" then 
        silent = true;
    elseif flag == "-d" then
        downloadSelected = avg[num+1]
        if downloadSelected == nil then
            warning = "The download link was not set correctly"
            writeData(nil,nil,nil,nil)
        end
        if socket.connect(downloadSelected,80) == nil then
            warning = "The download link was not set correctly"
            writeData(nil,nil,nil,nil)
        end
        tmp = 1
    elseif flag == "-u" then 
        uploadSelected = arg[num+1]
        if uploadSelected == nil then
            warning = "The upload link was not set correctly"
            writeData(nil,nil,nil,nil)
        end
        if socket.connect(downloadSelected,80) == nil then
            warning = "The upload link was not set correctly"
            writeData(nil,nil,nil,nil)
        end
        tmp = 1
    else
        print("The is no such option as "..flag)
    end
    return tmp
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

--If the dowload or upload server is already selected then find closes
if downloadSelected == nil then
    if not silent then
        print("Looking the best server to test download..")
    end
    downloadSelected = findClosestServer(downloadServers)
end

if uploadSelected == nil then
    if not silent then
        print("Looking the best server to test upload..")
    end
    uploadSelected = findClosestServer(uploadServers)
end

-- If the server was not able to determine which of the servers is closest puts random servers and a warning. 
if(not uploadSelected or not downloadSelected) then
    warning = "We could not determine the closes server to you. Random servers was picked."
    downloadSelected = downloadServers[math.random(1,#downloadServers)]
    uploadSelected = uploadServers[math.random(1,#uploadServers)]
end

if not silent then
    print("This speedtest can use up a lot of internet data. Do you want to continue?(Y/N)")
    local info = io.read();
    if not (info == "y") and not (info == "Y") then
        os.exit()
    end
end

isError, res = libspeedtest.testspeed(downloadSelected,2,false)
if isError then
    error = res
    writeData(nil,nil,nil,nil)
    os.exit()
end 
isError, res = libspeedtest.testspeed(uploadSelected,2,true)
if isError then
    error = res
    writeData(nil,nil,nil,nil)
    os.exit()
end

--gcc luaWrapper.c -shared -o libspeedtest.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
--gcc speedtest.c -shared -o libspeedtest.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
--/usr/lib/lua/luci/