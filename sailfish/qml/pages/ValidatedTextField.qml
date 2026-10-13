import QtQuick 2.6
import Sailfish.Silica 1.0

TextField {
    property string title
    property string error

    width: parent.width
    label: error !== "" ? error : title
    placeholderText: title
    errorHighlight: error !== ""
    EnterKey.iconSource: "image://theme/icon-m-enter-close"
    EnterKey.onClicked: focus = false
}
