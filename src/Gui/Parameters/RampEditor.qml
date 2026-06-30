import QtQuick
import Enzo

// Placeholder until the curve editor lands.
Rectangle {
    required property var item

    implicitHeight: 40
    radius: Theme.parameter.borderRadius
    color: Theme.parameter.backgroundColor
    border.color: Theme.parameter.lineColor

    Text {
        anchors.centerIn: parent
        text: "Ramp"
        color: Theme.var.textMuted
        font.family: Theme.var.fontSans
        font.pixelSize: 10
    }
}
