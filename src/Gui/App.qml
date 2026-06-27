import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "Spreadsheet"
import "Network"
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

    SplitView {
        id: split

        anchors.fill: parent
        orientation: Qt.Vertical

        // Invisible handle that still accepts drags to resize the sections.
        handle: Item {
            implicitHeight: 15
        }

        Item {
            SplitView.preferredHeight: Math.round(split.height * 0.6)

            Panel {
                anchors.fill: parent
                anchors.margins: 10
                anchors.bottomMargin: 0

                Network {
                    anchors.fill: parent
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
