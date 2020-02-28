map = Map("example_app", "Example APP")

section = map:section(NamedSection, "example_app", "example", "Example APP general")

flag = section:option(Flag, "enable", "Enable", "Enable program")

text = section:option( Value, "text", "Text")

return map

