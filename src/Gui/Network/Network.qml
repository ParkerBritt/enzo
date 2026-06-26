import QtQuick

Rectangle {
    id: root

    color: Theme.panel
    radius: Theme.panelRadius
    border.color: Theme.border
    clip: true

    property real viewZoom: 1
    property real viewX: 0
    property real viewY: 0
    property real mouseLastX: 0
    property real mouseLastY: 0
    property real zoomSpeed: 0.05;

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.MiddleButton

        onPressed: mouse => {
            root.mouseLastX = mouse.x;
            root.mouseLastY = mouse.y;
        }
        onPositionChanged: mouse => {
            viewX += mouse.x - root.mouseLastX;
            viewY += mouse.y - root.mouseLastY;
            root.mouseLastX = mouse.x;
            root.mouseLastY = mouse.y;
        }

        onWheel: wheel => {
            let oldZoom = root.viewZoom;
            let newZoom = oldZoom * (1 + Math.sign(wheel.angleDelta.y) * root.zoomSpeed);
            let zoomFactor = newZoom / oldZoom;

            console.log("mouse pos:", wheel.x, wheel.y);
            root.viewX = wheel.x - zoomFactor * (wheel.x - root.viewX);
            root.viewY = wheel.y - zoomFactor * (wheel.y - root.viewY);

            root.viewZoom = newZoom;
        }
    }

    Item {
        anchors.fill: parent

        transform: [
            Scale {
                xScale: root.viewZoom
                yScale: root.viewZoom
            },
            Translate {
                x: root.viewX
                y: root.viewY
            }
        ]

        // Demo node
        Rectangle {
            width: 80
            height: 20
            color: "red"
            radius: 5
            anchors.centerIn: parent
        }
    }
}
