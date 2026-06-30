import QtQuick
import QtQuick.Effects
import Enzo

Item {
    id: root

    width: network.nodeWidth
    height: network.nodeHeight

    // The node's position on the canvas, centered on the card.
    property real modelX: 0
    property real modelY: 0

    x: modelX - width / 2
    y: modelY - height / 2

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

    // While a link is being drawn the card lets presses through so the canvas can
    // grab and drop ports that sit beneath it.
    property bool linking: false

    // The slot index of the input or output port to highlight, or -1 for none.
    property int highlightedInputSlot: -1
    property int highlightedOutputSlot: -1

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

    // The primary node swaps its drop shadow for an accent glow.
    states: State {
        name: "primary"
        when: root.primary
        PropertyChanges {
            shadow.shadowColor: Theme.var.accent
            shadow.shadowBlur: 0.9
            shadow.shadowOpacity: 0.4
            shadow.shadowHorizontalOffset: 0
            shadow.shadowVerticalOffset: 0
        }
    }

    // The visible node card.
    Item {
        id: card

        anchors.fill: parent

        // Drag logic
        MouseArea {
            anchors.fill: parent
            enabled: !root.linking
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
            border.color: root.highlighted ? Theme.var.accent : root.borderColor
            border.width: (root.highlighted ? 1.5 : 0.8) / root.viewZoom
        }

        // Display flag inside the right edge, marking which node feeds the viewport.
        Rectangle {
            id: displayTab

            property real rightMargin: 3

            width: 6
            height: parent.height * 0.7
            radius: 2
            anchors.verticalCenter: parent.verticalCenter
            x: parent.width - rightMargin - width

            // Dim at rest, the flag brightens on hover and lights up when active.
            color: Theme.node.portColor
            opacity: 0.4
            states: [
                State {
                    name: "hovered"
                    when: flagMouse.containsMouse && !root.display
                    PropertyChanges {
                        displayTab.color: Theme.node.displayFlagColor
                        displayTab.opacity: 0.7
                    }
                },
                State {
                    name: "active"
                    when: root.display
                    PropertyChanges {
                        displayTab.color: Theme.node.displayFlagColor
                        displayTab.opacity: 1
                    }
                }
            ]
            Behavior on opacity {
                NumberAnimation {
                    duration: 90
                }
            }

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
    }

    // One port, a dot on its slot point along a card edge where the link layer
    // anchors its curves. Hit testing lives on the canvas, so this is purely the
    // visual dot and lights up only while it is the highlighted port.
    component Port: Item {
        id: portDot

        property int slotIndex: 0
        property int slotCount: 1
        property bool active: false

        x: root.width * (slotIndex + 1) / (slotCount + 1)

        Rectangle {
            width: 7
            height: 4
            radius: 1.5
            x: -width / 2
            y: -height / 2

            // The dot grows and brightens once it is the closest port, to invite a drag.
            color: portDot.active ? Qt.lighter(Theme.node.portColor, 1.6) : Theme.node.portColor
            scale: portDot.active ? 1.5 : 1
            Behavior on color {
                ColorAnimation {
                    duration: 90
                }
            }
            Behavior on scale {
                NumberAnimation {
                    duration: 90
                }
            }
        }
    }

    // Input ports along the top edge, output ports along the bottom.
    Repeater {
        model: root.inputSlotCount
        delegate: Port {
            required property int index
            slotIndex: index
            slotCount: root.inputSlotCount
            active: root.highlightedInputSlot === index
            y: 0
        }
    }
    Repeater {
        model: root.outputSlotCount
        delegate: Port {
            required property int index
            slotIndex: index
            slotCount: root.outputSlotCount
            active: root.highlightedOutputSlot === index
            y: root.height
        }
    }
}
