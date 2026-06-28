import QtQuick
import QtQuick.Effects

Item {
    id: root

    // The visible card size, the geometry the link layer attaches to.
    readonly property real cardWidth: Theme.nodeWidth
    readonly property real cardHeight: Theme.nodeHeight

    // Ports sit on the card edges, so the root extends past the card by this much
    // to keep each port and its hit area inside the bounds that receive mouse events.
    readonly property real portReach: 10

    width: cardWidth + portReach * 2
    height: cardHeight + portReach * 2

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

    // This node's operator id, so port drags can name their endpoint.
    property var opId

    // The canvas item that port positions are reported in.
    property Item canvas

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

    // Emitted when a port drag begins, carrying the grabbed port and its canvas point.
    signal portPressed(int slotIndex, bool isOutput, point canvasPoint)

    // Emitted as a port drag moves, carrying the cursor in canvas coordinates.
    signal portDragMoved(point canvasPoint)

    // Emitted as the cursor hovers a port, so a trailing link can snap to it.
    signal portHovered(point canvasPoint)

    // Emitted when a port drag is released, whether or not it found a target.
    signal portReleased

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

    // The visible node card, inset so the edge ports fall within the root bounds.
    Item {
        id: card

        x: root.portReach
        y: root.portReach
        width: root.cardWidth
        height: root.cardHeight

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
    }

    // One port, a draggable dot centered on its slot point along a card edge. The
    // link layer draws its curves to these same points, and dragging from a port
    // starts a new link.
    component Port: Item {
        id: portDot

        property int slotIndex: 0
        property int slotCount: 1
        property bool isOutput: false

        x: root.portReach + root.cardWidth * (slotIndex + 1) / (slotCount + 1)

        Rectangle {
            width: 6
            height: 6
            radius: 3
            x: -width / 2
            y: -height / 2
            color: Theme.nodePort

            // The dot grows under the cursor to invite a drag.
            scale: hit.containsMouse ? 1.5 : 1
            Behavior on scale { NumberAnimation { duration: 90 } }
        }

        MouseArea {
            id: hit
            width: 18
            height: 18
            x: -width / 2
            y: -height / 2
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton

            onPressed: mouse => {
                root.portPressed(
                    portDot.slotIndex,
                    portDot.isOutput,
                    portDot.mapToItem(root.canvas, 0, 0)
                );
                mouse.accepted = true;
            }
            onPositionChanged: mouse => {
                const canvasPoint = hit.mapToItem(root.canvas, mouse.x, mouse.y);
                if (hit.pressed)
                    root.portDragMoved(canvasPoint);
                else
                    root.portHovered(canvasPoint);
            }
            onReleased: root.portReleased()
        }
    }

    // Input ports along the top edge, output ports along the bottom.
    Repeater {
        model: root.inputSlotCount
        delegate: Port {
            required property int index
            slotIndex: index
            slotCount: root.inputSlotCount
            y: root.portReach
        }
    }
    Repeater {
        model: root.outputSlotCount
        delegate: Port {
            required property int index
            slotIndex: index
            slotCount: root.outputSlotCount
            isOutput: true
            y: root.portReach + root.cardHeight
        }
    }
}
