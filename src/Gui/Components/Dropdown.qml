import QtQuick
import Enzo

// Picker unrolling a list of choices below or above its trigger. The trigger
// names the current choice, or shows an icon where the list holds one off actions.
//
// e.g.
//   Dropdown {
//       labels: ["Linear", "Smooth"]
//       currentIndex: 0
//       onActivated: (index) => setMode(index)
//   }
Item {
    id: root

    property var labels: []
    property int currentIndex: -1

    // Icon standing in for the trigger text.
    property string icon: ""

    property string tooltip: ""

    // Whether the list unrolls above the trigger, for a picker near a panel floor.
    property bool opensUpward: false

    // Width of the list, which may exceed the trigger's.
    property real listWidth: Math.max(width, 150)

    signal activated(int index)

    readonly property string currentLabel:
        (currentIndex >= 0 && currentIndex < labels.length) ? labels[currentIndex] : ""

    // Room for the widest choice, so the trigger holds still as it changes.
    readonly property real widestLabelWidth: {
        let widest = 0
        for (const label of labels)
            widest = Math.max(widest, metrics.advanceWidth(label))
        return widest
    }

    implicitWidth: icon !== "" ? implicitHeight : Math.ceil(widestLabelWidth) + 40
    implicitHeight: 30

    function toggle() {
        if (list.visible)
            list.close()
        else
            list.open()
    }

    FontMetrics {
        id: metrics

        font.family: Theme.var.fontSans
        font.pixelSize: 12
        font.weight: Font.Medium
    }

    IconButton {
        anchors.fill: parent
        visible: root.icon !== ""
        variant: "field"
        name: root.icon
        tooltip: root.tooltip
        onClicked: root.toggle()
    }

    Rectangle {
        anchors.fill: parent
        visible: root.icon === ""
        radius: Theme.parameter.borderRadius
        color: Theme.parameter.backgroundColor
        border.color: list.visible ? Theme.var.accentLine : Theme.parameter.lineColor

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: chevron.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: root.currentLabel
            color: Theme.var.text
            font.family: Theme.var.fontSans
            font.pixelSize: 12
            font.weight: Font.Medium
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Icon {
            id: chevron

            name: "chevron-down"
            size: 12
            color: Theme.var.textMuted
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            rotation: list.visible ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        }

        MouseArea {
            id: labelMouse

            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.toggle()
        }

        Tooltip {
            text: root.tooltip
            visible: root.tooltip !== "" && labelMouse.containsMouse
        }
    }

    PopupList {
        id: list

        x: root.width - width
        y: root.opensUpward ? -height - 4 : root.height + 4
        implicitWidth: root.listWidth
        rowHeight: 30
        model: root.labels
        highlightedIndex: root.currentIndex
        onActivated: (index) => {
            root.activated(index)
            close()
        }

        delegate: Text {
            required property int index
            required property var modelData

            width: list.availableWidth
            height: list.rowHeight
            leftPadding: 10
            rightPadding: 10
            text: modelData
            color: index === root.currentIndex ? Theme.var.text : Theme.var.textLabel
            font.family: Theme.var.fontSans
            font.pixelSize: 12
            font.weight: Font.Medium
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
}
