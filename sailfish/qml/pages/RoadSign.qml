import QtQuick 2.6
import Sailfish.Silica 1.0

// The speed limit sign of Android's SpeedLimitView.
Rectangle {
    property alias text: label.text
    // Red fill: the speed limit is exceeded.
    property bool alert

    height: width
    radius: width / 2
    color: alert ? "#e53935" : "white"
    border.color: "#e53935"
    border.width: width * 0.1

    // Sized by the text's tight bounds like SpeedLimitView.configureTextSize, but with a smaller diagonal than its
    // 70%, which looks too large in the Silica font.
    TextMetrics {
        id: metrics
        font.bold: true
        font.pixelSize: 100
        text: label.text
    }

    Label {
        id: label
        anchors.centerIn: parent
        // Centers the glyphs rather than the line, which has room for descenders.
        anchors.verticalCenterOffset: {
            var tight = metrics.tightBoundingRect
            var line = metrics.boundingRect
            return (line.y + line.height / 2 - tight.y - tight.height / 2) * font.pixelSize / 100
        }
        color: alert ? "white" : "black"
        font.bold: true
        font.pixelSize: {
            var r = metrics.tightBoundingRect
            var diagonal = Math.sqrt(r.width * r.width + r.height * r.height)
            return diagonal > 0 ? Math.max(1, Math.floor(100 * 0.55 * parent.width / diagonal)) : Theme.fontSizeSmall
        }
    }
}
