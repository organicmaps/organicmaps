import QtQuick 2.6
import Sailfish.Silica 1.0
import "cover"
import "pages"

ApplicationWindow {
    id: appWindow

    property Item mapPage

    initialPage: Component {
        MapPage {
            id: mapPageItem
            Component.onCompleted: appWindow.mapPage = mapPageItem
        }
    }
    cover: Component {
        CoverPage {
            mapPage: appWindow.mapPage
        }
    }
    allowedOrientations: defaultAllowedOrientations

    Connections {
        target: urlHandler
        onActivated: activate()
    }
}
