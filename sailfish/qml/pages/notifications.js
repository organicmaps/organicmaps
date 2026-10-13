// Handled by UrlHandler. Not a .pragma library: it reads urlHandler.
function action(name, displayName, method, args) {
    return {
        "name": name,
        "displayName": displayName,
        "service": urlHandler.dbusService,
        "path": urlHandler.dbusPath,
        "iface": urlHandler.dbusService,
        "method": method,
        "arguments": args || []
    }
}

function openApp() {
    return action("default", "", "openUrl", [[]])
}
