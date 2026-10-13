.pragma library

function category(key, dark) {
    return Qt.resolvedUrl("../../icons/categories/ic_" + key + (dark ? "_night" : "") + ".svg")
}
