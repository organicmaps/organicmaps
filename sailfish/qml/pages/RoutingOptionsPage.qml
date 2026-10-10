import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

Page {
    property QtObject routing
    // The route is rebuilt once on leaving if the options changed.
    property int initialAvoidRoads
    property bool initialRouteOptimization

    function setAvoided(road, avoid) {
        routing.avoidRoads = avoid ? (routing.avoidRoads | road) : (routing.avoidRoads & ~road)
    }

    allowedOrientations: Orientation.All
    Component.onCompleted: {
        initialAvoidRoads = routing.avoidRoads
        initialRouteOptimization = routing.routeOptimization
    }
    onStatusChanged: {
        if (status === PageStatus.Deactivating)
            routing.applyOptions(initialAvoidRoads, initialRouteOptimization)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("driving_options_title")
            }
            Repeater {
                model: [
                    { road: Routing.Toll, text: appInfo.localized("avoid_tolls") },
                    { road: Routing.Dirty, text: appInfo.localized("avoid_unpaved") },
                    { road: Routing.Ferry, text: appInfo.localized("avoid_ferry") },
                    { road: Routing.Motorway, text: appInfo.localized("avoid_motorways") }
                ]

                TextSwitch {
                    text: modelData.text
                    checked: (routing.avoidRoads & modelData.road) !== 0
                    onClicked: setAvoided(modelData.road, checked)
                }
            }
            SectionHeader {
                text: appInfo.localized("route_optimization")
            }
            TextSwitch {
                text: appInfo.localized("route_optimization_description")
                checked: routing.routeOptimization
                onClicked: routing.routeOptimization = checked
            }
        }
    }
}
