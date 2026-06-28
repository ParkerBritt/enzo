import QtQuick

// Horizontal slider with a gradient fill and a value readout.
Item {
    id: root

    property real value: 0
    property real from: 0
    property real to: 1
    property bool integer: false
    signal moved(real value)

    implicitHeight: 22

    readonly property real fraction: to > from
        ? Math.max(0, Math.min(1, (value - from) / (to - from)))
        : 0

    Rectangle {
        id: track
        anchors.fill: parent
        radius: 6
        color: Theme.field
        border.color: Theme.fline

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width * root.fraction
            radius: 6
            color: Theme.accent
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.integer ? Math.round(root.value) : root.value.toFixed(3)
            color: Theme.text
            font.family: Theme.fontMono
            font.pixelSize: 12
        }
    }

    // Maps a press position to a value within the range.
    function emitAt(mouseX) {
        let f = Math.max(0, Math.min(1, mouseX / width))
        let v = from + f * (to - from)
        root.moved(integer ? Math.round(v) : v)
    }

    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => root.emitAt(mouse.x)
        onPositionChanged: (mouse) => { if (pressed) root.emitAt(mouse.x) }
    }
}
