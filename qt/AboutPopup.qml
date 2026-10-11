import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Map

Popup {
    id: aboutPopup
    required property Item rootItem
    required property MainWindow map
    property string copyrightText

    modal: true
    height: rootItem.height
    width: expandToolButton.checked || rootItem.height / rootItem.width > 16 / 9 ? rootItem.width : 400
    topPadding: 0

    Material.theme: parent.Material.theme
    Material.accent: parent.Material.accent
    Material.primary: parent.Material.primary

    onOpened:
        copyrightText = "<style>a { color: " + Material.accent + "; }</style>" + map.getCopyrightText()
    onClosed: copyrightText = ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            Layout.leftMargin: -aboutPopup.padding
            Layout.rightMargin: -aboutPopup.padding

            ToolButton {
                icon.name: "arrow-left-symbolic"
                onClicked: aboutPopup.close()
            }

            Label {
                text: "About " + Qt.application.name
                anchors.centerIn: parent
            }

            ToolButton {
                id: expandToolButton
                checkable: true
                anchors.right: parent.right
                icon.name: aboutPopup.width === aboutPopup.rootItem.width ? "arrow-left-symbolic" : "arrow-right-symbolic"
                visible: aboutPopup.rootItem.height / aboutPopup.rootItem.width < 16 / 9
            }
        }

        ScrollView {
            id: scrollView
            Layout.fillHeight: true
            Layout.fillWidth: true
            contentWidth: availableWidth

            Column {
                topPadding: 10
                spacing: 10

                Image {
                    source: "qrc:/ui/logo.png"
                    fillMode: Image.PreserveAspectFit
                    height: 100
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Label {
                    text: Qt.application.version
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 12
                }

                Label {
                    text: aboutPopup.copyrightText
                    wrapMode: Text.Wrap
                    width: scrollView.availableWidth
                    textFormat: Text.RichText
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }
            }
        }
    }
}