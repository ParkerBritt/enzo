import QtQuick
import Enzo
import "../Utils.js" as Utils

// Slim track dragged to set a whole number, such as the panel radius.
Item {
    id: root

    property real value: 0
    property real from: 0
    property real to: 24

    signal moved(real value)

    readonly property real fraction: (root.value - root.from) / (root.to - root.from)

    implicitWidth: 60
    implicitHeight: 12

    Rectangle {
        id: track

        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: 4
        radius: 2
        color: Theme.var.border

        Rectangle {
            width: Math.round(track.width * root.fraction)
            height: track.height
            radius: track.radius
            color: root.enabled ? Theme.var.accent : Theme.var.textMuted
        }
    }

    Rectangle {
        visible: root.enabled
        x: Math.round(track.width * root.fraction) - width / 2
        anchors.verticalCenter: parent.verticalCenter
        width: 10
        height: 10
        radius: 5
        color: Theme.var.textStrong
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: -6
        enabled: root.enabled

        // Sets the value from the cursor. The area reaches 6px past the track,
        // so the position shifts back by 6 before it is read as a fraction.
        function setFromCursor(x) {
            const fraction = Utils.clamp((x - 6) / track.width, 0, 1);
            root.moved(Math.round(root.from + fraction * (root.to - root.from)));
        }

        onPressed: mouse => setFromCursor(mouse.x)
        onPositionChanged: mouse => setFromCursor(mouse.x)
    }
}
