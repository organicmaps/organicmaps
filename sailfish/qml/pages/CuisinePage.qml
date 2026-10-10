import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property QtObject placeEditor
    // Keys separated by ';', changed when accepted.
    property string value
    property var selected: value !== "" ? value.split(";") : []

    readonly property var allCuisines: placeEditor.cuisines()

    allowedOrientations: Orientation.All
    onAccepted: value = selected.join(";")

    // Kept outside the list: a new model would take the focus from a field in its header.
    Column {
        id: header
        width: parent.width

        DialogHeader {
            title: appInfo.localized("select_cuisine")
        }
        SearchField {
            id: searchField
            width: parent.width
            placeholderText: appInfo.localized("search_in_the_list")
        }
    }

    SilicaListView {
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        model: {
            var query = searchField.text.trim().toLowerCase()
            return query === "" ? dialog.allCuisines : dialog.allCuisines.filter(function(cuisine) {
                return cuisine.name.toLowerCase().indexOf(query) >= 0 || cuisine.key.indexOf(query) >= 0
            })
        }
        currentIndex: -1

        delegate: TextSwitch {
            text: modelData.name
            automaticCheck: false
            checked: dialog.selected.indexOf(modelData.key) >= 0
            onClicked: {
                var selected = dialog.selected.slice()
                if (checked)
                    selected.splice(selected.indexOf(modelData.key), 1)
                else
                    selected.push(modelData.key)
                dialog.selected = selected
            }
        }

        VerticalScrollDecorator {}
    }
}
