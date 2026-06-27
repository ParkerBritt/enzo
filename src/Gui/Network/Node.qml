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

    // The primary and selected nodes both carry an accent outline.
    readonly property bool highlighted: selected || primary

    // Emitted on mouse down, additive when a modifier is held.
    signal pressed(bool additive)

    // Emitted on a release that was a click rather than a drag.
    signal clicked(bool additive)

    // Emitted each frame of a drag with the move since the last frame.
    signal dragMoved(real dx, real dy)

    // Emitted when a drag finishes, so the new position can be committed.
    signal dragReleased()

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
        x: -40
        y: 0
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

    Text {
        text: root.label
        color: "white"
        font.pointSize: 5
        anchors.verticalCenter: parent.verticalCenter
        x: 10
    }
}
