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

    // Drag logic
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        property real dragStartX: 0
        property real dragStartY: 0

        onPressed: mouse => {
            dragStartX = mouse.x;
            dragStartY = mouse.y;
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

    Text {
        text: root.label
        color: "white"
        font.pointSize: 5
        anchors.verticalCenter: parent.verticalCenter
        x: 10
    }
}
