import QtQuick
import Enzo
import "../Style"

// Horizontal slider that opens for typing on a click, or scrubs relative to drag distance.
Item {
    id: root

    property real value: 0
    property real from: 0
    property real to: 1
    property bool integer: false
    property bool clampMin: true
    property bool clampMax: true
    property bool editing: false
    signal moved(real value)
    signal expressionEntered(string expression)

    signal pressed
    signal released

    implicitHeight: Constants.parameterHeight

    readonly property real fraction: to > from
        ? Math.max(0, Math.min(1, (value - from) / (to - from)))
        : 0

    // Tracks the unrounded value through a drag so sub-integer pixel deltas
    // still accumulate instead of getting rounded away on every step.
    property real dragValue: 0

    function beginDrag() {
        dragValue = value
    }

    function applyDelta(pixelDelta) {
        dragValue += (pixelDelta / width) * (to - from)
        if (clampMin) dragValue = Math.max(from, dragValue)
        if (clampMax) dragValue = Math.min(to, dragValue)
        root.moved(integer ? Math.round(dragValue) : dragValue)
    }

    function beginEdit() {
        editField.text = root.integer ? Math.round(root.value).toString() : root.value.toFixed(3)
        root.editing = true
        editField.selectAll()
        editField.forceActiveFocus()
    }

    // A typed number commits as a value, any other text commits as an expression.
    function commitEdit() {
        if (!root.editing) return
        root.editing = false
        const text = editField.text.trim()
        if (text.length === 0) return

        const parsed = Number(text)
        if (isNaN(parsed)) {
            root.expressionEntered(text)
            return
        }

        let v = parsed
        if (clampMin) v = Math.max(from, v)
        if (clampMax) v = Math.min(to, v)
        root.pressed()
        root.moved(integer ? Math.round(v) : v)
        root.released()
    }

    function cancelEdit() {
        root.editing = false
    }

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
            visible: !root.editing
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.integer ? Math.round(root.value) : root.value.toFixed(3)
            color: Theme.var.text
            font.family: Theme.var.fontMono
            font.pixelSize: 12
        }

        TextInput {
            id: editField
            visible: root.editing
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            verticalAlignment: TextInput.AlignVCenter
            horizontalAlignment: TextInput.AlignHCenter
            clip: true
            selectByMouse: true
            color: Theme.var.text
            font.family: Theme.var.fontMono
            font.pixelSize: 12
            onAccepted: root.commitEdit()
            onEditingFinished: root.commitEdit()
            Keys.onEscapePressed: root.cancelEdit()
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.editing
        cursorShape: Qt.SizeHorCursor
        property point pressPos
        property real lastX
        property bool dragging: false

        onPressed: (mouse) => {
            pressPos = Qt.point(mouse.x, mouse.y)
            lastX = mouse.x
            dragging = false
        }
        onPositionChanged: (mouse) => {
            if (!dragging) {
                if (Math.abs(mouse.x - pressPos.x) < 3) return
                dragging = true
                root.beginDrag()
                root.pressed()
            }
            root.applyDelta(mouse.x - lastX)
            lastX = mouse.x
        }
        onReleased: {
            if (dragging) {
                dragging = false
                root.released()
            } else {
                root.beginEdit()
            }
        }
    }
}
