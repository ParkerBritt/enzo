import QtQuick

Rectangle {
    id: root

    color: Theme.panel
    radius: Theme.panelRadius
    border.color: Theme.border
    clip: true

    // How fast scrolling changes the zoom.
    property real zoomSpeed: 0.2
    // How far you can zoom in. (e.g. 5x the initial scale)
    property real zoomMax: 5
    // How far you can zoom out. (e.g. 0.1x the initial scale)
    property real zoomMin: 0.1
    // Default zoom scale.
    property real viewZoom: 1

    property real viewX: width/2
    property real viewY: height/2
    property real mouseLastX: 0
    property real mouseLastY: 0

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
            let oldZoomScale = root.viewZoom;
            let newZoomScale = oldZoomScale * (1 + Math.sign(wheel.angleDelta.y) * root.zoomSpeed);
            // Clamp zoom
            newZoomScale = Math.min(Math.max(newZoomScale, root.zoomMin), root.zoomMax);
            let scaleFactor = newZoomScale / oldZoomScale;

            root.viewX = wheel.x - scaleFactor * (wheel.x - root.viewX);
            root.viewY = wheel.y - scaleFactor * (wheel.y - root.viewY);

            root.viewZoom = newZoomScale;
        }
    }

    ShaderEffect {
        width: root.width
        height: root.height
        fragmentShader: "qrc:/NetworkDots.frag.qsb"

        property real zoom: root.viewZoom
        property point pan: Qt.point(root.viewX, root.viewY)
        property size canvas: Qt.size(width, height)
        property color dotColor: Theme.networkDot
    }

    // Canvas
    Item {
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
            x: -40
            y: 0
        }
        Rectangle {
            width: 80
            height: 20
            color: "red"
            radius: 5
            x: 1000
            y: 1000
        }
    }
}
