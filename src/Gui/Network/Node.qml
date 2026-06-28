import QtQuick
import QtQuick.Effects

Item {
    id: root

    width: 80
    height: 25
    property color fillColor: "#1f202a"
    property color borderColor: "#333341"

    property string label: "Grid"
    property real radius: 5
    property real viewZoom: 1
    property bool selected: false
    property bool primary: false
    property bool display: false
    property int inputSlotCount: 0
    property int outputSlotCount: 0

    // The primary and selected nodes both carry an accent outline.
    readonly property bool highlighted: selected || primary

    // Emitted when the display flag is clicked, to show this node's geometry.
    signal displayToggled

    // Emitted on mouse down, additive when a modifier is held.
    signal pressed(bool additive)

    // Emitted on a release that was a click rather than a drag.
    signal clicked(bool additive)

    // The offset between the cursor and the grab point since last frame.
    signal dragMoved(real dx, real dy)

    // Emitted when a drag finishes, so the new position can be committed.
    signal dragReleased

    // Drag logic
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        property real dragStartX: 0
        property real dragStartY: 0
        property bool dragged: false
        property bool pressAdditive: false

        onPressed: mouse => {
            dragStartX = mouse.x;
            dragStartY = mouse.y;
            dragged = false;
            pressAdditive = (mouse.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) !== 0;
            root.pressed(pressAdditive);
        }
        onPositionChanged: mouse => {
            root.dragMoved(mouse.x - dragStartX, mouse.y - dragStartY);
            dragged = true;
        }
        onReleased: {
            if (dragged)
                root.dragReleased();
            else
                root.clicked(pressAdditive);
        }
    }

    // Drop Shadow
    Rectangle {
        id: dropShadowRect
        anchors.fill: parent
        radius: root.radius
        color: root.borderColor
    }
    MultiEffect {
        id: shadow
        source: dropShadowRect
        anchors.fill: dropShadowRect
        shadowEnabled: true
        shadowBlur: 0.4
        shadowOpacity: 0.4
        shadowHorizontalOffset: 2
        shadowVerticalOffset: 2
    }

    // Main shape
    Rectangle {
        id: nodeShape
        anchors.fill: parent
        color: root.fillColor
        radius: root.radius
        antialiasing: true
        border.pixelAligned: false
        border.color: root.highlighted ? Theme.accent : root.borderColor
        border.width: (root.highlighted ? 1.5 : 0.8) / root.viewZoom
    }

    // The primary node swaps its drop shadow for an accent glow.
    states: State {
        name: "primary"
        when: root.primary
        PropertyChanges {
            shadow.shadowColor: Theme.accent
            shadow.shadowBlur: 0.9
            shadow.shadowOpacity: 0.4
            shadow.shadowHorizontalOffset: 0
            shadow.shadowVerticalOffset: 0
        }
    }

    // Display flag inside the right edge, marking which node feeds the viewport.
    Rectangle {
        id: displayTab

        property real rightMargin: 3

        width: 6
        height: parent.height * 0.7
        radius: 2
        anchors.verticalCenter: parent.verticalCenter
        x: root.width - rightMargin - width

        // Dim at rest, the flag brightens on hover and lights up when active.
        color: Theme.nodePort
        opacity: 0.4
        states: [
            State {
                name: "hovered"
                when: flagMouse.containsMouse && !root.display
                PropertyChanges {
                    displayTab.color: Theme.displayFlag
                    displayTab.opacity: 0.7
                }
            },
            State {
                name: "active"
                when: root.display
                PropertyChanges {
                    displayTab.color: Theme.displayFlag
                    displayTab.opacity: 1
                }
            }
        ]
        Behavior on opacity { NumberAnimation { duration: 90 } }

        MouseArea {
            id: flagMouse
            anchors.fill: parent
            anchors.margins: -4
            hoverEnabled: true
            onClicked: root.displayToggled()
        }
    }

    Text {
        text: root.label
        color: "white"
        font.pointSize: 5
        anchors.verticalCenter: parent.verticalCenter
        x: 10
    }

    // One port dot, centered on its slot spread evenly across the node width.
    // The link layer draws its curves to these same points.
    component Port: Rectangle {
        property int slotIndex: 0
        property int slotCount: 1
        width: 6
        height: 6
        radius: 3
        color: Theme.nodePort
        x: root.width * (slotIndex + 1) / (slotCount + 1) - width / 2
    }

    // Input ports along the top edge, output ports along the bottom.
    Repeater {
        model: root.inputSlotCount
        delegate: Port {
            required property int index
            slotIndex: index
            slotCount: root.inputSlotCount
            y: -height / 2
        }
    }
    Repeater {
        model: root.outputSlotCount
        delegate: Port {
            required property int index
            slotIndex: index
            slotCount: root.outputSlotCount
            y: root.height - height / 2
        }
    }
}
