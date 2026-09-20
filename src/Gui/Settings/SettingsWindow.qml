import QtQuick
import QtQuick.Window
import Enzo
import "../Components"

// The settings window, one page per section listed down the left.
Window {
    id: root

    property int currentPage: 0

    readonly property var pages: [
        {
            title: "Theme",
            icon: "palette",
            source: "ThemePage.qml"
        },
        {
            title: "Keymap",
            icon: "keyboard",
            source: "KeymapPage.qml"
        },
    ]

    width: 1040
    height: 720
    minimumWidth: 720
    minimumHeight: 460
    title: "Enzo Settings"
    color: Theme.var.background

    // The list of pages.
    Rectangle {
        id: sidebar

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 7
        width: 196
        radius: Theme.var.panelRadius
        color: Theme.var.surfaceHeader
        border.color: Theme.var.border

        Column {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 2

            Repeater {
                model: root.pages

                delegate: Rectangle {
                    id: entry

                    required property int index
                    required property var modelData

                    readonly property bool current: root.currentPage === entry.index

                    width: parent.width
                    height: 28
                    radius: 8
                    color: entry.current ? Theme.var.selectedFill : hover.hovered ? Theme.var.borderSoft : "transparent"

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 9

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: entry.modelData.icon
                            size: 14
                            color: entry.current ? Theme.var.textStrong : Theme.var.textLabel
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: entry.modelData.title
                            color: entry.current ? Theme.var.textStrong : Theme.var.textLabel
                            font.family: Theme.var.fontSans
                            font.pixelSize: 12
                            font.weight: entry.current ? Font.DemiBold : Font.Medium
                        }
                    }

                    HoverHandler {
                        id: hover
                    }

                    TapHandler {
                        onTapped: root.currentPage = entry.index
                    }
                }
            }
        }
    }

    Panel {
        anchors.left: sidebar.right
        anchors.leftMargin: 7
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 7

        Loader {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: footer.top
            source: root.pages[root.currentPage].source
        }

        // The bar that applies or discards the current page's edits.
        Item {
            id: footer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 52

            Rectangle {
                anchors.fill: parent
                color: Theme.var.surfaceHeader

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1
                    color: Theme.var.borderSoft
                }
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: "No pending changes"
                color: Theme.var.textMuted
                font.family: Theme.var.fontSans
                font.pixelSize: 12
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                TextButton {
                    text: "Revert"
                    enabled: false
                }

                TextButton {
                    text: "Apply"
                    variant: "accent"
                    enabled: false
                }
            }
        }
    }
}
