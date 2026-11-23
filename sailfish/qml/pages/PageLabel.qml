import QtQuick 2.6
import Sailfish.Silica 1.0

// Text wrapped within the page margins.
Label {
    x: Theme.horizontalPageMargin
    width: parent.width - 2 * x
    wrapMode: Text.Wrap
}
