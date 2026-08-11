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

    // Second action every row offers, revealed under the cursor.
    property string rowActionLabel: ""
    property string rowActionIcon: ""

    signal activated(int index)
    signal rowActionActivated(int index)

    readonly property string currentLabel: (currentIndex >= 0 && currentIndex < labels.length) ? labels[currentIndex] : ""

    // Room for the widest choice, so the trigger holds still as it changes.
    readonly property real widestLabelWidth: {
        let widest = 0;
        for (const label of labels)
            widest = Math.max(widest, metrics.advanceWidth(label));
        return widest;
    }

    implicitWidth: icon !== "" ? implicitHeight : Math.ceil(widestLabelWidth) + 40
    implicitHeight: 30

    // Returns the row under a point given in this item's coordinates, or -1 when
    // the point misses the open list.
    function rowAt(px, py) {
        if (!list.visible)
            return -1;

        // An upward list unrolls past the cursor, so a point still on the trigger
        // names no row.
        if (px >= 0 && px <= width && py >= 0 && py <= height)
            return -1;

        const localX = px - list.x - list.padding;
        const localY = py - list.y - list.padding;
        return (localX >= 0 && localX <= list.availableWidth) ? list.rowAt(localY) : -1;
    }

    function choose(index) {
        root.activated(index);
        list.close();
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
            Behavior on rotation {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.OutCubic
                }
            }
        }

        HoverHandler {
            id: labelHover
        }

        Tooltip {
            text: root.tooltip
            visible: root.tooltip !== "" && labelHover.hovered
        }
    }

    // One area over whichever trigger is showing, opening the list on a press and
    // carrying that press onto a row. Hover belongs to the trigger beneath.
    MouseArea {
        anchors.fill: parent

        // A press on an open list already closed it on the way past.
        onPressed: if (!list.visible)
            list.open()

        onPositionChanged: mouse => {
            const index = root.rowAt(mouse.x, mouse.y);
            if (index >= 0)
                list.highlightedIndex = index;
        }
        onReleased: mouse => {
            const index = root.rowAt(mouse.x, mouse.y);
            if (index >= 0)
                root.choose(index);
        }
    }

    PopupList {
        id: list

        x: root.width - width
        y: root.opensUpward ? -height - 4 : root.height + 4
        implicitWidth: root.listWidth
        rowHeight: 30
        model: root.labels

        // The list opens on the current choice, then the highlight follows the cursor.
        onAboutToShow: highlightedIndex = root.currentIndex

        onActivated: index => root.choose(index)

        delegate: Item {
            id: row

            required property int index
            required property var modelData

            readonly property bool underCursor: index === list.highlightedIndex

            width: list.availableWidth
            height: list.rowHeight

            Text {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: (rowAction.visible ? rowAction.width + 14 : 10) + (selectionDot.visible ? 14 : 0)
                text: row.modelData
                color: row.index === root.currentIndex ? Theme.var.text : Theme.var.textLabel
                font.family: Theme.var.fontSans
                font.pixelSize: 12
                font.weight: Font.Medium
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            Rectangle {
                id: selectionDot

                visible: row.index === root.currentIndex
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 7
                height: 7
                radius: 3.5
                color: Theme.var.accentBright
            }

            Rectangle {
                id: rowAction

                visible: root.rowActionLabel !== ""
                opacity: row.underCursor ? 1 : 0
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                width: actionBody.width + 14
                height: 22
                radius: 7
                // Opaque, so the fill hides the selection dot beneath it.
                color: actionMouse.containsMouse ? Theme.var.accent : Theme.parameter.backgroundColor

                Behavior on opacity {
                    NumberAnimation {
                        duration: 120
                    }
                }

                Row {
                    id: actionBody

                    anchors.centerIn: parent
                    spacing: 5

                    Icon {
                        visible: root.rowActionIcon !== ""
                        name: root.rowActionIcon
                        size: 12
                        color: Theme.var.accentBright
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.rowActionLabel
                        color: Theme.var.text
                        font.family: Theme.var.fontSans
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: actionMouse

                    anchors.fill: parent
                    enabled: row.underCursor
                    hoverEnabled: true
                    onClicked: {
                        root.rowActionActivated(row.index);
                        list.close();
                    }
                }
            }
        }
    }
}
