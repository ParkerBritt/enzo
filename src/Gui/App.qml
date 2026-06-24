import QtQuick
import QtQuick.Controls
import QtQuick.Window
import "Spreadsheet"

// Root window of the QML application.
ApplicationWindow {
    id: window

    width: 1280
    height: 800
    visible: true
    visibility: Window.Maximized
    title: "Enzo"
    color: "#0a0a0d"

    Spreadsheet {
        anchors.fill: parent
        anchors.margins: 7
        viewModel: spreadsheet
    }
}
