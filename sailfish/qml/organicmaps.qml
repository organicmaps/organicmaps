import QtQuick 2.6
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: appWindow

    initialPage: Component {
        MapPage {}
    }
    allowedOrientations: defaultAllowedOrientations

    Connections {
        target: urlHandler
        onActivated: activate()
    }
}
