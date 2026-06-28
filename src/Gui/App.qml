import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "Spreadsheet"
import "Network"
import "Viewport"
import "Parameters"
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
    property real marginSize: 15

    Shortcut {
        sequence: StandardKey.Undo
        onActivated: network.undo()
    }
    Shortcut {
        sequence: StandardKey.Redo
        onActivated: network.redo()
    }

    Item
    {
        id: panels
        anchors.fill: parent
        anchors.margins: window.marginSize

        SplitView {
            id: split

            anchors.fill: parent
            orientation: Qt.Vertical

            // Invisible handle that still accepts drags to resize the sections.
            handle: Item {
                implicitHeight: window.marginSize
            }

            Item {
                SplitView.preferredHeight: Math.round(split.height * 0.7)

                // Top row splits the network and the viewport side by side.
                SplitView {
                    id: topRow

                    anchors.fill: parent
                    orientation: Qt.Horizontal

                    handle: Item {
                        implicitWidth: window.marginSize
                    }

                    Item {
                        SplitView.preferredWidth: Math.round(topRow.width * 0.63)

                        Panel {
                            anchors.fill: parent

                            Network {
                                anchors.fill: parent
                            }
                        }

                        // Parameters float over the network at its top right.
                        ParametersPanel {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: 14
                        }
                    }

                    Item {
                        Panel {
                            anchors.fill: parent

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
                    Spreadsheet {
                        viewModel: spreadsheet
                        anchors.fill: parent
                    }
                }
            }
        }
    }
}
