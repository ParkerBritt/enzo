import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "Spreadsheet"
import "Network"
import "Viewport"
import "Components"

// Root window of the QML application.
ApplicationWindow {
    id: window

    width: 1280
    height: 800
    visible: true
    visibility: Window.Maximized
    title: "Enzo"
    color: "#0a0a0d"

    Shortcut {
        sequence: StandardKey.Undo
        onActivated: network.undo()
    }
    Shortcut {
        sequence: StandardKey.Redo
        onActivated: network.redo()
    }

    SplitView {
        id: split

        anchors.fill: parent
        orientation: Qt.Vertical

        // Invisible handle that still accepts drags to resize the sections.
        handle: Item {
            implicitHeight: 15
        }

        Item {
            SplitView.preferredHeight: Math.round(split.height * 0.7)

            // Top row splits the network and the viewport side by side.
            SplitView {
                id: topRow

                anchors.fill: parent
                orientation: Qt.Horizontal

                handle: Item {
                    implicitWidth: 15
                }

                Item {
                    SplitView.preferredWidth: Math.round(topRow.width * 0.63)

                    Panel {
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.bottomMargin: 0
                        anchors.rightMargin: 5

                        Network {
                            anchors.fill: parent
                        }
                    }
                }

                Item {
                    Panel {
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.bottomMargin: 0
                        anchors.leftMargin: 5

                        Viewport {
                            anchors.fill: parent
                        }
                    }
                }
            }
        }

        Item {
            Panel {
                anchors.fill: parent
                anchors.margins: 10
                anchors.topMargin: 0
                Spreadsheet {
                    viewModel: spreadsheet
                    anchors.fill: parent
                }
            }
        }
    }
}
