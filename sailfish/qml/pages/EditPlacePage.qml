import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "editor.js" as Editor
import "navigation.js" as Navigation
import "notices.js" as Toast

Dialog {
    id: dialog

    // MapPage holds links back while the user types here.
    property bool keepsUserInput: true
    property string street
    // A new place of this type at lat, lon, from the category page; otherwise the selected place.
    property string newPlaceType
    property real lat
    property real lon
    // Names in other languages as {code, language, value}, including the ones added here.
    property var names: []
    readonly property var fieldRepeaters: [addressFields, detailFields, socialFields, buildingFields]
    // For the field delegates: TextField has an "editor" property of its own that hides the id.
    readonly property alias placeEditor: editor

    function valid() {
        if (nameField.visible && nameField.error !== "")
            return false
        for (var n = 0; n < namesRepeater.count; ++n)
            if (namesRepeater.itemAt(n).error !== "")
                return false
        if (addressColumn.visible && houseField.error !== "")
            return false
        for (var r = 0; r < fieldRepeaters.length; ++r) {
            for (var i = 0; i < fieldRepeaters[r].count; ++i) {
                var field = fieldRepeaters[r].itemAt(i)
                if (field && field.item && field.item.error !== "")
                    return false
            }
        }
        return true
    }

    function fieldsOf(section) {
        return editor.fields.filter(function(field) { return field.section === section })
    }

    function save() {
        if (editor.nameEditable) {
            editor.name = nameField.text
            for (var n = 0; n < namesRepeater.count; ++n)
                editor.setLocalizedName(names[n].code, namesRepeater.itemAt(n).text)
        }
        if (editor.addressEditable) {
            editor.street = street
            editor.houseNumber = houseField.text
        }
        for (var r = 0; r < fieldRepeaters.length; ++r) {
            for (var i = 0; i < fieldRepeaters[r].count; ++i) {
                var field = fieldRepeaters[r].itemAt(i)
                editor.setField(field.fieldId, field.item.fieldValue)
            }
        }
        if (editor.save()) {
            editor.createNote(noteField.text)
            osmAccount.uploadChanges()
        } else {
            // E.g. the map was updated meanwhile.
            Toast.show(appInfo.localized("dialog_routing_system_error"))
        }
    }

    allowedOrientations: Orientation.All
    canAccept: editor.valid && valid()
    Component.onCompleted: {
        if (newPlaceType !== "")
            editor.create(newPlaceType, lat, lon)
        else
            editor.start()
        street = editor.street
        names = editor.localizedNames
        if (!appSettings.editsPublicNoticeShown) {
            // Saved only once the notice is accepted; cancelling it returns here.
            acceptDestination = Qt.resolvedUrl("MessageDialog.qml")
            acceptDestinationAction = PageStackAction.Push
            acceptDestinationProperties = Editor.publicEditNotice(function() {
                save()
                if (osmAccount.loggedIn)
                    pageStack.pop(pageStack.previousPage(dialog))
                else
                    pageStack.replace(Qt.resolvedUrl("OsmAccountPage.qml"))
            })
        } else if (!osmAccount.loggedIn) {
            acceptDestination = Qt.resolvedUrl("OsmAccountPage.qml")
            acceptDestinationAction = PageStackAction.Replace
        }
    }
    onAccepted: {
        if (appSettings.editsPublicNoticeShown)
            save()
    }

    PlaceEditor {
        id: editor
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        ViewPlaceholder {
            enabled: !editor.valid
            text: dialog.newPlaceType !== "" ? appInfo.localized("message_invalid_feature_position")
                                             : appInfo.localized("editor_category_unsuitable_title")
        }

        Column {
            id: column
            width: parent.width
            visible: editor.valid

            DialogHeader {
                title: dialog.newPlaceType !== "" ? appInfo.localized("editor_add_place_title")
                                                  : appInfo.localized("editor_edit_place_title")
                acceptText: appInfo.localized("save")
                cancelText: appInfo.localized("cancel")
            }
            PageLabel {
                text: appInfo.localized("editor_about_osm")
                textFormat: Text.StyledText
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                horizontalAlignment: Text.AlignHCenter
                bottomPadding: Theme.paddingLarge
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("editor_edit_place_category_title")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
            PageLabel {
                text: editor.category
                color: Theme.highlightColor
                bottomPadding: Theme.paddingMedium
            }

            Column {
                width: parent.width
                visible: editor.nameEditable

                SectionHeader {
                    text: appInfo.localized("editor_edit_place_name_hint")
                }
                ValidatedTextField {
                    id: nameField
                    text: dialog.placeEditor.name
                    title: appInfo.localized("place_name")
                    error: dialog.placeEditor.nameError(text)
                    placeholderText: appInfo.localized("editor_default_language_hint")
                }
                Repeater {
                    id: namesRepeater
                    model: dialog.names

                    ValidatedTextField {
                        text: modelData.value
                        title: modelData.language
                        error: dialog.placeEditor.nameError(text)
                    }
                }
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: appInfo.localized("add_language")
                    onClicked: {
                        var used = dialog.names.map(function(name) { return name.code })
                        var languages = editor.otherLanguages().filter(function(language) {
                            return used.indexOf(language.code) < 0
                        })
                        pageStack.push(Qt.resolvedUrl("ListPickerPage.qml"), {
                            title: appInfo.localized("choose_language"),
                            items: languages.map(function(language) { return { name: language.language } }),
                            picked: function(index) {
                                // Keeps the typed names: the Repeater recreates its fields for the new model.
                                var names = []
                                for (var n = 0; n < namesRepeater.count; ++n)
                                    names.push({ code: dialog.names[n].code, language: dialog.names[n].language,
                                                 value: namesRepeater.itemAt(n).text })
                                names.push({ code: languages[index].code, language: languages[index].language,
                                             value: "" })
                                dialog.names = names
                            }
                        })
                    }
                }
            }

            Column {
                id: addressColumn
                width: parent.width
                visible: editor.addressEditable

                SectionHeader {
                    text: appInfo.localized("address")
                }
                EditorRow {
                    icon: "../../icons/editor/ic_street_address.svg"

                    ValueButton {
                        width: parent.width
                        label: appInfo.localized("street")
                        value: dialog.street !== "" ? dialog.street : appInfo.localized("choose_street")
                        onClicked: {
                            var page = pageStack.push(Qt.resolvedUrl("StreetPage.qml"),
                                                      { streets: editor.nearbyStreets, current: dialog.street })
                            page.selected.connect(function(street) { dialog.street = street })
                        }
                    }
                }
                EditorRow {
                    icon: "../../icons/editor/ic_building.svg"

                    ValidatedTextField {
                        id: houseField
                        text: dialog.placeEditor.houseNumber
                        title: appInfo.localized("house_number")
                        error: dialog.placeEditor.houseNumberError(text)
                        inputMethodHints: Qt.ImhNoPredictiveText
                    }
                }
                Repeater {
                    id: addressFields
                    model: dialog.fieldsOf(PlaceEditor.Address)
                    delegate: fieldDelegate
                }
            }

            SectionHeader {
                visible: detailFields.count > 0
                text: appInfo.localized("details")
            }
            Repeater {
                id: detailFields
                model: dialog.fieldsOf(PlaceEditor.Details)
                delegate: fieldDelegate
            }

            SectionHeader {
                visible: socialFields.count > 0
                text: appInfo.localized("social_media")
            }
            Repeater {
                id: socialFields
                model: dialog.fieldsOf(PlaceEditor.SocialMedia)
                delegate: fieldDelegate
            }

            SectionHeader {
                visible: buildingFields.count > 0
                text: appInfo.localized("building")
            }
            Repeater {
                id: buildingFields
                model: dialog.fieldsOf(PlaceEditor.Building)
                delegate: fieldDelegate
            }

            SectionHeader {
                text: appInfo.localized("editor_other_info")
            }
            TextArea {
                id: noteField
                width: parent.width
                placeholderText: appInfo.localized("editor_note_hint")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: editor.resetAction !== PlaceEditor.NoReset
                text: {
                    switch (editor.resetAction) {
                    case PlaceEditor.RemovePlace: return appInfo.localized("editor_remove_place_button")
                    case PlaceEditor.PlaceDoesntExist: return appInfo.localized("editor_place_doesnt_exist")
                    default: return appInfo.localized("editor_reset_edits_button")
                    }
                }
                onClicked: {
                    if (editor.resetAction === PlaceEditor.PlaceDoesntExist) {
                        pageStack.push(Qt.resolvedUrl("PlaceDoesntExistDialog.qml"),
                                       { placeEditor: editor, returnPage: pageStack.previousPage(dialog) })
                        return
                    }
                    var message = editor.resetAction === PlaceEditor.RemovePlace
                            ? appInfo.localized("editor_remove_place_message")
                            : appInfo.localized("editor_reset_edits_message")
                    Remorse.popupAction(dialog, message, function() {
                        editor.reset()
                        osmAccount.updateEdits()
                        Navigation.popPage(pageStack, dialog)
                    })
                }
            }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: fieldDelegate

        EditorRow {
            readonly property int fieldId: modelData.id
            readonly property alias item: loader.item

            icon: "../../icons/editor/" + modelData.icon + ".svg"

            Loader {
                id: loader
                // The loaded components see the field through their parent, not modelData.
                readonly property var field: modelData

                width: parent.width
                sourceComponent: {
                    switch (modelData.kind) {
                    case PlaceEditor.Wifi: return wifiField
                    case PlaceEditor.SelfService: return selfServiceField
                    case PlaceEditor.OpeningHours: return openingHoursField
                    case PlaceEditor.Cuisine: return cuisineField
                    case PlaceEditor.Phone: return phoneField
                    case PlaceEditor.YesNo: return yesNoField
                    default: return textField
                    }
                }
            }
        }
    }

    // Each field editor exposes the value to save and an error message for an invalid one.
    Component {
        id: textField

        ValidatedTextField {
            readonly property string fieldValue: text

            text: parent.field.value
            title: parent.field.label
            error: dialog.placeEditor.fieldError(parent.field.id, text)
            inputMethodHints: {
                switch (parent.field.inputHint) {
                case "url": return Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                case "email": return Qt.ImhEmailCharactersOnly
                case "phone": return Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText
                case "number": return Qt.ImhDigitsOnly
                // Levels can be negative, fractional or lists like "0;1".
                case "level": return Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText
                default: return Qt.ImhNone
                }
            }
        }
    }
    Component {
        id: wifiField

        TextSwitch {
            readonly property string fieldValue: checked ? "yes" : ""
            readonly property string error: ""

            text: parent.field.label
            checked: parent.field.value === "yes"
        }
    }
    Component {
        id: selfServiceField

        ComboBox {
            readonly property var values: dialog.placeEditor.selfServiceValues()
            readonly property string fieldValue: currentIndex > 0 ? values[currentIndex - 1].value : ""
            readonly property string error: ""

            label: parent.field.label
            currentIndex: {
                for (var i = 0; i < values.length; ++i)
                    if (values[i].value === parent.field.value)
                        return i + 1
                return 0
            }
            menu: ContextMenu {
                MenuItem { text: "—" }
                Repeater {
                    model: values
                    MenuItem { text: modelData.name }
                }
            }
        }
    }
    Component {
        id: yesNoField

        ComboBox {
            readonly property var values: ["", "yes", "no"]
            readonly property string fieldValue: values[currentIndex]
            readonly property string error: ""

            label: parent.field.label
            currentIndex: Math.max(0, values.indexOf(parent.field.value))
            menu: ContextMenu {
                MenuItem { text: "—" }
                MenuItem { text: appInfo.localized("yes") }
                MenuItem { text: appInfo.localized("no") }
            }
        }
    }
    Component {
        id: openingHoursField

        ValueButton {
            id: openingHoursButton

            property string fieldValue: parent.field.value
            readonly property string error: dialog.placeEditor.fieldError(parent.field.id, fieldValue)

            label: parent.field.label
            value: fieldValue !== "" ? dialog.placeEditor.openingHoursText(fieldValue) : "—"
            valueColor: error !== "" ? Theme.errorColor : Theme.highlightColor
            onClicked: {
                var page = pageStack.push(Qt.resolvedUrl("OpeningHoursPage.qml"), { value: fieldValue })
                page.accepted.connect(function() { openingHoursButton.fieldValue = page.value })
            }
        }
    }
    Component {
        id: cuisineField

        ValueButton {
            id: cuisineButton

            property string fieldValue: parent.field.value
            readonly property string error: ""

            label: parent.field.label
            value: fieldValue !== "" ? dialog.placeEditor.cuisineNames(fieldValue) : appInfo.localized("select_cuisine")
            onClicked: {
                var page = pageStack.push(Qt.resolvedUrl("CuisinePage.qml"),
                                          { placeEditor: dialog.placeEditor, value: fieldValue })
                page.accepted.connect(function() { cuisineButton.fieldValue = page.value })
            }
        }
    }
    Component {
        id: phoneField

        Column {
            id: phoneColumn

            property var phones: parent.field.value !== "" ? parent.field.value.split(";") : [""]
            readonly property var fieldId: parent.field.id
            readonly property string fieldValue: phones.map(function(phone) { return phone.trim() })
                                                       .filter(function(phone) { return phone !== "" }).join(";")
            readonly property string error: dialog.placeEditor.fieldError(fieldId, fieldValue)
            readonly property string label: parent.field.label

            width: parent.width

            Repeater {
                model: phoneColumn.phones.length

                ValidatedTextField {
                    width: phoneColumn.width - (removeButton.visible ? removeButton.width : 0)
                    text: phoneColumn.phones[index]
                    title: phoneColumn.label
                    error: dialog.placeEditor.fieldError(phoneColumn.fieldId, text)
                    inputMethodHints: Qt.ImhDialableCharactersOnly | Qt.ImhNoPredictiveText
                    onTextChanged: {
                        var phones = phoneColumn.phones.slice()
                        phones[index] = text
                        phoneColumn.phones = phones
                    }

                    IconButton {
                        id: removeButton
                        anchors.left: parent.right
                        visible: phoneColumn.phones.length > 1
                        icon.source: "image://theme/icon-m-remove"
                        onClicked: {
                            var phones = phoneColumn.phones.slice()
                            phones.splice(index, 1)
                            phoneColumn.phones = phones
                        }
                    }
                }
            }
            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeSmall
                onClicked: phoneColumn.phones = phoneColumn.phones.concat([""])

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: appInfo.localized("editor_add_phone")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }
        }
    }
}
