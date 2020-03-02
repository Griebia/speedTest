map = Map("speed_test", "Speed test")

section = map:section(NamedSection, "speed_test", "speed", "Speedtest")

flag = section:option(Flag, "enable", "Enable", "Enable program")

text = section:option( Value, "text", "Text")

return map

