mylib = require 'mylib'
local socket =  require("socket")
local https = require("ssl.https")

-- Counts the lenght of an array.
function tableLength(T)
    local count = 0
    for _ in pairs(T) do count = count + 1 end
    return count
end

-- Gets the current ip address latitude and longitude.
function getCurrentLocation()
    -- Getting the json file form https://api.myip.com with IP.
    local body, code, headers, status = https.request("https://api.myip.com")
    -- Getting the IP from the json file.
    ip = parseIP(body)
    -- Gets the infomation about the ip address.
    body, code, headers, status = https.request("https://ipapi.co/" .. ip .. "/json")
    -- Gets from the retuned data the latitude and longitude.
    return parseLocation(body)
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
    local body, code, headers, status = https.request("https://ipapi.co/" .. ip .. "/json")
    -- Gets from the retuned data the latitude and longitude.
    return parseLocation(body)
end

-- Parse the latitude and longitude form given string.
function parseLocation(body)
    local i,j = string.find(body, "latitude")
    i = string.find(body,"\n",j)
    latitude = string.sub(body,j+3,i-2)
    local i,j = string.find(body, "longitude")
    i = string.find(body,"\n",j)
    longitude = string.sub(body,j+3,i-2)
    return tonumber(latitude), tonumber(longitude)
end

--Trims the link by removing https:// or http:// and removing everything from the link that goes from / character.
function trimLink(link)
    local i, j = string.find(link, "//")
    i = string.find(link, "\n")
    local cutLink = string.sub(link, j+1, i)
    i = string.find(cutLink, "/")
    cutLink = string.sub(cutLink, 0, i-1)
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
            print(minDistance)
            print(server)
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

function printSpeeds (avarageSpeed, currentDownloaded)
    print("Avarage speed is "..avarageSpeed.." Current downloaded "..currentDownloaded)
    return (x^2 * math.sin(y))/(1 - x)
  end


local servers = {
    "http://speedtest.tele2.net/10GB.zip",
    "http://mirror.nl.leaseweb.net/speedtest/1000mb.bin",
    "http://speedtest.as5577.net/1000mb.bin",
    "http://mirror.sfo12.us.leaseweb.net/speedtest/1000mb.bin",
    "http://mirror.wdc1.us.leaseweb.net/speedtest/1000mb.bin"
}

local client = socket.connect( "mirror.sfo12.us.leaseweb.net", 80 )
local ip, port = client:getpeername() 
print(ip)

--local server = findCloses(servers)

-- print(server)
i,j = getCurrentLocation()
-- i, j = getLocation(servers[1])
print(i,j)

i, j = getLocation(servers[1])
print(i,j)

-- Get the ip address of the site
local client = socket.connect( "mirror.sfo12.us.leaseweb.net", 80 )
local ip, port = client:getpeername() 
print(ip)

-- Gets my ip address
local body, code, headers, status = https.request("https://api.myip.com")
print(body)

-- Gets the ip address information
body, code, headers, status = https.request("https://ipapi.co/88.119.152.93/json")
print(body)


local selected = 1  

print(mylib.curl(servers[selected]))
-- curl -o newtxt.txt  http://speedtest.tele2.net/10GB.zip 2> informationFile.txt


-- local ok, statusCode, headers, statusText = https.request {
--     method = "GET",
--     url = "https://ipapi.co/88.119.152.93/json",
--     sink = collect
--   }
--   print("ok\t",         ok);
-- print("statusCode", statusCode)
-- print("statusText", statusText)
-- print("headers:")
-- for i,v in pairs(headers) do
--     print("\t",i, v)
--   end





--gcc luaWrapper.c -shared -o mylib.so -fPIC -llua5.2 -I/usr/include/lua5.2/ -lcurl
