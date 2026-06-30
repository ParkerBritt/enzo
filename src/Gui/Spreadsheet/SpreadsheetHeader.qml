import QtQuick
import Enzo
import "../Components"

// Top bar of the spreadsheet drawer: title, breadcrumb path, component mode
// pill, and the find and collapse controls.
Rectangle {
    id: root

    property var viewModel
    readonly property var path: ["obj", "characters", "tommy", "geo", "scatter_setup", "scatter1"]

    color: Theme.var.surfaceHeader

    function pathColor(index)
    {
        if (index === 0)
            return "#6a6a74"
        if (index === path.length -1)
            return Theme.var.accentBright
        return "#aaaab2"
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.var.borderSoft
    }

    // Title, breadcrumb.
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 13
        anchors.right: rightCluster.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            name: "table"
            size: 15
            color: Theme.var.accentBright
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "SPREADSHEET"
            color: Theme.var.textLabel
            font.family: Theme.var.fontSans
            font.pixelSize: 10
            font.weight: Font.Bold
            font.letterSpacing: 1.4
        }

        // Spacer
        Item { width: 7; height: 1 }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5

            Repeater {
                model: root.path

                delegate: Row {
                    id: crumb

                    required property int index
                    required property string modelData
                    spacing: 5

                    Text {
                        text: "/"
                        color: "#39394a"
                        font.family: Theme.var.fontMono
                        font.pixelSize: 11
                    }
                    Text {
                        text: crumb.modelData
                        color: pathColor(crumb.index)
                        font.family: Theme.var.fontMono
                        font.pixelSize: 11
                    }
                }
            }
        }
    }

    // Owner mode control, find button, and collaps button, on the right.
    Row {
        id: rightCluster

        anchors.right: parent.right
        anchors.rightMargin: 11
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        ModeControl {
            anchors.verticalCenter: parent.verticalCenter
            mode: root.viewModel.mode
            onModePicked: mode => root.viewModel.mode = mode
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 27
            height: 27
            radius: 7
            color: "transparent"
            border.color: Theme.var.border

            Icon {
                anchors.centerIn: parent
                name: "search"
                size: 14
                color: Theme.var.textLabel
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: collapse.width + 22
            height: 27
            radius: 7
            color: Theme.var.accentDim

            Row {
                id: collapse

                anchors.centerIn: parent
                spacing: 7

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Collapse"
                    color: Theme.var.accentBright
                    font.family: Theme.var.fontSans
                    font.pixelSize: 11
                    font.weight: Font.Medium
                }
                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron-down"
                    size: 13
                    color: Theme.var.accentBright
                }
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 1
                    height: 11
                    color: Qt.rgba(0.655, 0.545, 0.98, 0.25)
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "S"
                    color: Qt.rgba(0.655, 0.545, 0.98, 0.55)
                    font.family: Theme.var.fontMono
                    font.pixelSize: 10
                }
            }
        }
    }
}
