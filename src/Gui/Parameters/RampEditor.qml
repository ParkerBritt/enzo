import QtQuick

// Placeholder until the curve editor lands.
Rectangle {
    required property var item

    implicitHeight: 40
    radius: Theme.parameterRadius
    color: Theme.parameterBg
    border.color: Theme.parameterLine

    Text {
        anchors.centerIn: parent
        text: "Ramp"
        color: Theme.muted
        font.family: Theme.fontUi
        font.pixelSize: 10
    }
}
