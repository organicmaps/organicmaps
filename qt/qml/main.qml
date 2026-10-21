import QtQuick
import QtQuick.Controls.Material
import Map

Item {
    id: root
    required property MainWindow map
    property bool windowIsVertical: root.height / root.width > 16 / 9
    // style_utils::NightMode Off/On/System equal Material.Light/Dark/System (0/1/2).
    Material.theme: map.nightMode
}
