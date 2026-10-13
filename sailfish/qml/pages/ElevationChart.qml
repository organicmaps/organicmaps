import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: root

    // ElevationChart() data of the C++ side: {profile, length, min, max}; the profile has distance and altitude
    // pairs in one flat list.
    property var elevation: ({})
    readonly property var profile: elevation.profile || []
    // Distances of the marked point and of the position, -1 without them.
    property real activePoint: -1
    property real myPosition: -1
    signal pointClicked(real distance)

    width: parent.width
    height: Theme.itemSizeHuge * 1.2
    visible: profile.length >= 4

    Label {
        id: maxLabel
        x: Theme.horizontalPageMargin
        anchors.top: chart.top
        text: root.elevation.max || ""
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.secondaryColor
    }
    Label {
        x: Theme.horizontalPageMargin
        anchors.bottom: chart.bottom
        text: root.elevation.min || ""
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.secondaryColor
    }

    Canvas {
        id: chart

        property real minAltitude
        property real maxAltitude
        readonly property real length: Math.max(1, root.elevation.length || 0)

        function xOf(distance) { return distance / length * width }
        function yOf(altitude) {
            return height - (altitude - minAltitude) / Math.max(1, maxAltitude - minAltitude) * height
        }
        // Linear between the profile points.
        function altitudeAt(distance) {
            var p = root.profile
            for (var i = 2; i < p.length; i += 2) {
                if (p[i] >= distance) {
                    var span = p[i] - p[i - 2]
                    return span > 0 ? p[i - 1] + (p[i + 1] - p[i - 1]) * (distance - p[i - 2]) / span : p[i + 1]
                }
            }
            return p[p.length - 1]
        }

        anchors {
            left: maxLabel.right
            leftMargin: Theme.paddingMedium
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            top: parent.top
            topMargin: Theme.paddingMedium
            bottom: parent.bottom
            bottomMargin: Theme.paddingMedium
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var p = root.profile
            if (p.length < 4)
                return
            var min = p[1], max = p[1]
            for (var i = 3; i < p.length; i += 2) {
                min = Math.min(min, p[i])
                max = Math.max(max, p[i])
            }
            minAltitude = min
            maxAltitude = max

            ctx.beginPath()
            ctx.moveTo(xOf(p[0]), height)
            for (i = 0; i < p.length; i += 2)
                ctx.lineTo(xOf(p[i]), yOf(p[i + 1]))
            ctx.lineTo(xOf(p[p.length - 2]), height)
            ctx.closePath()
            ctx.fillStyle = Theme.rgba(Theme.highlightColor, 0.2)
            ctx.fill()

            ctx.beginPath()
            ctx.moveTo(xOf(p[0]), yOf(p[1]))
            for (i = 2; i < p.length; i += 2)
                ctx.lineTo(xOf(p[i]), yOf(p[i + 1]))
            ctx.lineWidth = Theme.dp(2)
            ctx.strokeStyle = Theme.highlightColor
            ctx.stroke()
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        Connections {
            target: root
            onProfileChanged: chart.requestPaint()
        }

        Rectangle {
            visible: root.activePoint >= 0
            x: chart.xOf(root.activePoint) - width / 2
            width: Theme.dp(2)
            height: parent.height
            color: Theme.primaryColor
        }
        Rectangle {
            visible: root.myPosition >= 0 && root.profile.length >= 4
            x: chart.xOf(root.myPosition) - width / 2
            y: chart.yOf(chart.altitudeAt(root.myPosition)) - height / 2
            width: Theme.paddingMedium
            height: width
            radius: width / 2
            color: Theme.highlightColor
            border.color: Theme.primaryColor
            border.width: Theme.dp(1)
        }

        MouseArea {
            anchors.fill: parent
            onClicked: report(mouse.x)
            onPositionChanged: report(mouse.x)

            function report(x) { root.pointClicked(Math.max(0, Math.min(x, chart.width)) / chart.width * chart.length) }
        }
    }
}
