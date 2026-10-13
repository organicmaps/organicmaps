import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTrId("backup")
            }
            PageLabel {
                text: qsTrId("backup_description")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                bottomPadding: Theme.paddingLarge
            }
            // Set only when chosen, so that loading the page keeps a period not offered here.
            ComboBox {
                id: periodBox
                // Days between backups.
                readonly property var periods: [0, 1, 7]

                label: qsTrId("backup_automatic")
                currentIndex: periods.indexOf(bookmarksIO.backupPeriod)
                menu: ContextMenu {
                    Repeater {
                        model: ["off", "daily", "backup_weekly"]
                        MenuItem {
                            text: qsTrId(modelData)
                            onClicked: bookmarksIO.backupPeriod = periodBox.periods[index]
                        }
                    }
                }
            }
            ValueButton {
                label: qsTrId("backup_folder")
                value: bookmarksIO.backupFolder
                valueColor: Theme.secondaryHighlightColor
                onClicked: pageStack.push(folderPicker)
            }
            Item {
                width: 1
                height: Theme.paddingLarge
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTrId("backup_now")
                enabled: !bookmarksIO.backingUp
                onClicked: bookmarksIO.backUpNow()
            }
            PageLabel {
                topPadding: Theme.paddingLarge
                horizontalAlignment: Text.AlignHCenter
                text: isNaN(bookmarksIO.lastBackup.getTime())
                      ? qsTrId("backup_none")
                      : qsTrId("backup_last").arg(Format.formatDate(bookmarksIO.lastBackup,
                                                                            Formatter.DateMedium))
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: folderPicker

        FolderPickerPage {
            dialogTitle: qsTrId("backup_folder")
            onSelectedPathChanged: bookmarksIO.backupFolder = selectedPath
        }
    }
}
