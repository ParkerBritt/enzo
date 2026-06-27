import QtQuick
import QtQuick.Controls
import "../Utils.js" as Utils
import "."

Rectangle {
    id: root

    color: Theme.panel
    radius: Theme.panelRadius
    border.color: Theme.border
    focus: true
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

    // Latest cursor position over the network, used to place popups.
    property real cursorX: 0
    property real cursorY: 0

    // Maps a view position to its position on the panned and zoomed canvas.
    function toCanvasX(viewPosX) { return (viewPosX - viewX) / viewZoom; }
    function toCanvasY(viewPosY) { return (viewPosY - viewY) / viewZoom; }

    Keys.onTabPressed: (event) => {
        tabMenu.x = root.cursorX;
        tabMenu.y = root.cursorY;
        tabMenu.open()
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace)
            network.deleteSelected();
    }

    // Pan and zoom navigations
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.MiddleButton | Qt.LeftButton
        hoverEnabled: true

        onPressed: mouse => {
            root.mouseLastX = mouse.x;
            root.mouseLastY = mouse.y;
        }

        // A left click on empty canvas clears the selection. Clicks on a node
        // are consumed by the node, so they never reach here.
        onClicked: mouse => {
            if (mouse.button === Qt.LeftButton)
                network.clearSelection();
        }
        onPositionChanged: mouse => {
            root.cursorX = mouse.x;
            root.cursorY = mouse.y;

            // Panning only happens while the middle button is held.
            if (!(mouse.buttons & Qt.MiddleButton))
                return;

            viewX += mouse.x - root.mouseLastX;
            viewY += mouse.y - root.mouseLastY;
            root.mouseLastX = mouse.x;
            root.mouseLastY = mouse.y;
        }

        onWheel: wheel => {
            let oldZoomScale = root.viewZoom;
            let newZoomScale = oldZoomScale * (1 + Math.sign(wheel.angleDelta.y) * root.zoomSpeed);
            // Clamp zoom
            newZoomScale = Utils.clamp(newZoomScale, root.zoomMin, root.zoomMax);
            let scaleFactor = newZoomScale / oldZoomScale;

            root.viewX = wheel.x - scaleFactor * (wheel.x - root.viewX);
            root.viewY = wheel.y - scaleFactor * (wheel.y - root.viewY);

            root.viewZoom = newZoomScale;
        }
    }

    // Background dots
    ShaderEffect {
        width: root.width
        height: root.height
        fragmentShader: "qrc:/NetworkDots.frag.qsb"

        property real zoom: root.viewZoom
        property point pan: Qt.point(root.viewX, root.viewY)
        property size canvas: Qt.size(width, height)
        property color dotColor: Theme.networkDot
    }

    // Tab menu
    TabMenu {
        id: tabMenu
        nodeTypes: network.nodeTypes
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onNodeTypeChosen: name => network.createNode(name, root.toCanvasX(x), root.toCanvasY(y))
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

        Repeater {
            model: network.nodes

            delegate: Node {
                id: nodeDelegate
                viewZoom: root.viewZoom
                x: model.x
                y: model.y
                label: model.name
                selected: model.selected
                primary: model.primary

                // Was this node already selected when the press began.
                property bool selectedAtPress: false

                onPressed: additive => {
                    selectedAtPress = model.selected;
                    if (!model.selected)
                        network.selectNode(model.opId, additive);
                }
                onClicked: additive => {
                    if (selectedAtPress)
                        network.selectNode(model.opId, additive);
                }
                onDragMoved: (dx, dy) => network.stageSelectionMove(dx, dy)
                onDragReleased: network.commitSelectionMove()
            }
        }
    }
}
