.import app.organicmaps 1.0 as OM

// The name of a bookmark list.
function listName(id) {
    var lists = bookmarksIO.lists
    for (var i = 0; i < lists.length; ++i) {
        if (lists[i].id === id)
            return lists[i].name
    }
    return ""
}

// The bookmark list chooser with "Add a new list": picked gets the id of the chosen or new list.
// The current list is marked.
function pickList(pageStack, currentId, picked) {
    var lists = bookmarksIO.lists
    pageStack.push(Qt.resolvedUrl("ListPickerPage.qml"), {
        title: appInfo.localized("select_list"),
        items: lists.map(function(list) { return { name: list.name, selected: list.id === currentId } }),
        picked: function(index) { picked(lists[index].id) },
        addText: appInfo.localized("add_new_set"),
        addAction: function() {
            pageStack.replace(Qt.resolvedUrl("NewListDialog.qml"), {
                createAction: function(name) { picked(bookmarksIO.createList(name)) }
            })
        }
    })
}

// The export formats; exportFile gets the BookmarksIO.FileType chosen, e.g. to call bookmarksIO.exportList().
function pickExportFormat(pageStack, exportFile) {
    var formats = [{ key: "export_file", type: OM.BookmarksIO.Kmz },
                   { key: "export_file_gpx", type: OM.BookmarksIO.Gpx },
                   { key: "export_file_geojson", type: OM.BookmarksIO.GeoJson }]
    pageStack.push(Qt.resolvedUrl("ListPickerPage.qml"), {
        title: appInfo.localized("share"),
        items: formats.map(function(format) { return { name: appInfo.localized(format.key) } }),
        picked: function(index) { exportFile(formats[index].type) }
    })
}
