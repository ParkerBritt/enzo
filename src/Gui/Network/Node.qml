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

    // Emitted on a left click, additive when a modifier extends the selection.
    signal clicked(bool additive)

    // Drag logic
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        property real dragStartX: 0
        property real dragStartY: 0

        onPressed: mouse => {
            dragStartX = mouse.x;
            dragStartY = mouse.y;
            let additive = (mouse.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) !== 0;
            root.clicked(additive);
        }
        onPositionChanged: mouse => {
            root.x += mouse.x - dragStartX;
            root.y += mouse.y - dragStartY;
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
        border.color: root.borderColor
        border.width: 0.8 / root.viewZoom
    }

    // A selected node swaps its drop shadow for a centred violet glow and an
    // accent border.
    states: State {
        name: "selected"
        when: root.selected
        PropertyChanges {
            shadow.shadowColor: Theme.accent
            shadow.shadowBlur: 0.9
            shadow.shadowOpacity: 0.7
            shadow.shadowHorizontalOffset: 0
            shadow.shadowVerticalOffset: 0
            nodeShape.border.color: Theme.accent
            nodeShape.border.width: 1.5 / root.viewZoom
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
