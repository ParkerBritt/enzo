import QtQuick

// Placeholder until the curve editor lands.
Rectangle {
    required property var item

    implicitHeight: 40
    radius: 6
    color: Theme.field
    border.color: Theme.fline

    Text {
        anchors.centerIn: parent
        text: "Ramp"
        color: Theme.muted
        font.family: Theme.fontUi
        font.pixelSize: 10
    }
}
