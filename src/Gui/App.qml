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

        Item
        {
            SplitView.preferredHeight: split.height * 0.6
            Network {
                anchors.fill: parent
                anchors.margins: 10
            }
        }

        Item
        {
            Panel
            {
                anchors.fill: parent
                anchors.margins: 10
                Spreadsheet {
                    viewModel: spreadsheet
                    anchors.fill: parent
                }
            }
        }
    }
}
