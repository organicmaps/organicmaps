import QtQuick 2.6
import Sailfish.Silica 1.0

Column {
    // MapItem.placePage.
    property QtObject placePage

    width: parent.width
    bottomPadding: Theme.paddingMedium

    Grid {
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        columns: 2
        rowSpacing: Theme.paddingSmall

        Repeater {
            model: placePage.trackStats

            Column {
                width: parent.width / 2

                Label {
                    width: parent.width
                    text: modelData.label
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    text: modelData.value
                    color: Theme.highlightColor
                    truncationMode: TruncationMode.Fade
                }
            }
        }
    }

    ElevationChart {
        elevation: placePage.elevation
        activePoint: placePage.elevationActivePoint
        myPosition: placePage.elevationMyPosition
        onPointClicked: placePage.setElevationActivePoint(distance)
    }
}
