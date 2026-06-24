import QtQuick
import QtQuick.Controls
import QtQuick.Window

// Root window of the QML application.
ApplicationWindow {
    id: window

    width: 1280
    height: 800
    visible: true
    visibility: Window.Maximized
    title: "Enzo"
    color: "#0a0a0d"

    Label {
        anchors.centerIn: parent
        text: "Enzo — QML shell"
        color: "#e7e8ec"
        font.pixelSize: 18
    }
}
