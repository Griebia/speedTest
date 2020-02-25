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

locationAPIs = {
    "https://ipapi.co/{ip}/json/",
    "extreme-ip-lookup.com/json/{ip}",
    "http://ip-api.com/json/{ip}",
    "https://ipgeolocation.com/{ip}",
    "https://api.ipgeolocationapi.com/geolocate/{ip}"
}

locationAPISelected = 1


-- Counts the lenght of an array.
function tableLength(T)
    local count = 0
    for _ in pairs(T) do count = count + 1 end
    return count
end

-- Gets the current ip address latitude and longitude.
function getCurrentLocation()
    -- Getting the json file form https://api.myip.com with IP.
    local body = libspeedtest.get_body("https://api.myip.com")
    -- Getting the IP from the json file.
    ip = parseIP(body)
    -- Gets the infomation about the ip address.
    body = libspeedtest.get_body(string.gsub(locationAPIs[locationAPISelected],"{ip}",ip))
    if (string.match(body,"lib") or string.match(body, "coords")) then
        -- Gets from the retuned data the latitude and longitude.
        return parseLocation(body)
    end
    print(body)
    locationAPISelected = locationAPISelected + 1
    return getCurrentLocation()
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
    local ip, port = client:getpeername()
    -- Gets the ip address information.
    local body = libspeedtest.get_body(string.gsub(locationAPIs[locationAPISelected],"{ip}",ip))
    
    if (string.match(body,"lib") or string.match(body, "coords")) then
        -- Gets from the retuned data the latitude and longitude.
        return parseLocation(body)
    end

    locationAPISelected = locationAPISelected + 1
    return getLocation(link)
end

-- Parse the latitude and longitude form given string.
function parseLocation(body)
    print(body)
    if string.match(body,"coords") then
        local i,j = string.find(body, "coords")
        local _,_, lat1, lat2, lon1, lon2 = string.find(body,"(%d+).(%d+),(%d+).(%d+)",j)
        latitude = lat1.."."..lat2
        longitude = lon1.."."..lon2
    else
        local i,j = string.find(body, "lat")
        local _,_, integer, decimal = string.find(body,"(%d+).(%d+)",j)
        print(integer,decimal)
        local latitude = integer.."."..decimal
        i,j = string.find(body, "lat")
        _,_, integer, decimal = string.find(body,"(%d+).(%d+)",j)
        local longitude = integer.."."..decimal
        
    end
    print(latitude,longitude);
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

-- Finds the closes server from the current location.
function findCloses(servers)
    local server;
    local lat1, lon1 = getCurrentLocation()
    local count = tableLength(servers)
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

function printSpeeds (downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    if currentDownloaded > 0 then
        print("Avarage download speed is "..convertBytes(downloadSpeed).."/s Current downloaded "..convertBytes(currentDownloaded))
    else
        print("Avarage upload speed is "..convertBytes(uploadSpeed).."/s Current uploaded "..convertBytes(currentUpload)) 
    end
    writeToFile(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    return true
end

function writeToFile(downloadSpeed, currentDownloaded,uploadSpeed,currentUpload)
    if currentDownloaded > 0 then
        file:write("Avarage download speed is "..convertBytes(downloadSpeed).."/s Current downloaded "..convertBytes(currentDownloaded).."\n")
    else
        file:write("Avarage upload speed is "..convertBytes(uploadSpeed).."/s Current uploaded "..convertBytes(currentUpload).."\n")
    end
end

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



if(#arg == 1) then
    local uploadServer = findCloses(uploadServers)
    local downloadServer = findCloses(downloadServers)
    print(uploadServer)
    print(downloadServer)
end

-- file = io.open("speedtest.txt","w")

-- local selected = 1  

-- print("This speedtest can use up a lot of internet data. Do you want to continue?(Y/N)")
-- local info = io.read();
-- if info == "y" or info == "Y" then
--     libspeedtest.curl("http://mirror.nl.leaseweb.net/speedtest/1000mb.bin",10,false)
--         print()
--     libspeedtest.curl("https://de.testmy.net/uploader",10,true)
-- end

-- file:close()
--gcc luaWrapper.c -shared -o libspeedtest.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
--gcc curlWrap.c -shared -o libspeedtest.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
--/usr/lib/lua/luci/