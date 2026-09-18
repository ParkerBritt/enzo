import QtQuick
import Enzo

// The hint shown on a network with no nodes in it. It sits on the panel colour
// so the background dots do not run through the text.
Rectangle {
    width: hintText.width + 20
    height: hintText.height + 12
    color: Theme.var.surface
    radius: 6

    Text {
        id: hintText
        anchors.centerIn: parent
        text: "Press Tab to place a node"
        color: Theme.var.textMuted
        font.family: Theme.var.fontSans
        font.pixelSize: 13
    }
}
