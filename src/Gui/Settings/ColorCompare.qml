import QtQuick
import Enzo

// One labelled colour of the pair the picker compares.
Row {
    id: root

    property string label: ""
    property color color: "transparent"

    spacing: 6

    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: Theme.var.textMuted
        font.family: Theme.var.fontMono
        font.pixelSize: 10
    }

    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: 58
        height: 18
        radius: 5
        color: root.color
        border.color: Qt.rgba(1, 1, 1, 0.16)
    }
}
