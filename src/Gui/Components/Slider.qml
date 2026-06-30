import QtQuick
import Enzo

// Horizontal slider with a gradient fill and a value readout.
Item {
    id: root

    property real value: 0
    property real from: 0
    property real to: 1
    property bool integer: false
    property bool clampMin: true
    property bool clampMax: true
    signal moved(real value)

    implicitHeight: 22

    readonly property real fraction: to > from
        ? Math.max(0, Math.min(1, (value - from) / (to - from)))
        : 0

    Rectangle {
        id: track
        anchors.fill: parent
        radius: Theme.parameter.borderRadius
        color: Theme.parameter.backgroundColor
        border.color: Theme.parameter.lineColor

        // The accent fill floats inside the frame with a small inset on every
        // side so the rounded track border stays visible around it.
        Rectangle {
            id: fill
            readonly property real inset: 3
            x: fill.inset
            y: fill.inset
            height: parent.height - fill.inset * 2
            width: (parent.width - fill.inset * 2) * root.fraction
            radius: Theme.parameter.borderRadius - fill.inset
            color: Theme.slider.fillColor
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.integer ? Math.round(root.value) : root.value.toFixed(3)
            color: Theme.var.text
            font.family: Theme.var.fontMono
            font.pixelSize: 12
        }
    }

    // Maps a press position to a value. An unlocked end lets a drag past the
    // track exceed the soft range, while a locked end pins the value at it.
    function emitAt(mouseX) {
        let f = mouseX / width
        if (clampMin) f = Math.max(0, f)
        if (clampMax) f = Math.min(1, f)
        let v = from + f * (to - from)
        root.moved(integer ? Math.round(v) : v)
    }

    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => root.emitAt(mouse.x)
        onPositionChanged: (mouse) => { if (pressed) root.emitAt(mouse.x) }
    }
}
