module("luci.controller.example_app", package.seeall)

function index()
	entry( {"admin", "services", "example_app"}, cbi("example_app"), _("Example APP"), 100)
end

