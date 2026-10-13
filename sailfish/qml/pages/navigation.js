.pragma library

function popToMap(pageStack) {
    pageStack.pop(pageStack.find(function(page) { return page.objectName === "mapPage" }))
}

// A delayed action can't use pop(), which closes the page on top.
function popPage(pageStack, page) {
    var below = pageStack.previousPage(page)
    if (below)
        pageStack.pop(below)
}
