import QtQuick
import QtQuick.Controls
import Enzo
import "../Utils.js" as Utils
import "."

Rectangle {
    id: root

    color: Theme.var.surface
    radius: Theme.var.panelRadius
    border.color: Theme.var.border
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

    property real viewX: width / 2
    property real viewY: height / 2
    property real mouseLastX: 0
    property real mouseLastY: 0

    // Latest cursor position over the network, used to place popups.
    property real cursorX: 0
    property real cursorY: 0

    // Maps a view position to its position on the panned and zoomed canvas.
    function toCanvasX(viewPosX) {
        return (viewPosX - viewX) / viewZoom;
    }
    function toCanvasY(viewPosY) {
        return (viewPosY - viewY) / viewZoom;
    }

    // The port shown highlighted, the one a press would act on. While a link is
    // drawn this is its snap target, otherwise the port nearest the idle cursor.
    readonly property var highlightedPort: {
        if (linkController.linking) {
            if (linkController.hoverOpId === undefined)
                return null;
            return {
                opId: linkController.hoverOpId,
                slot: linkController.hoverSlot,
                isOutput: !linkController.fromOutput
            };
        }
        const port = network.nodes.getGrabPort(Qt.point(toCanvasX(cursorX), toCanvasY(cursorY)));
        return port.opId === undefined ? null : port;
    }

    // Holds the state of the link being dragged between ports.
    NodeLinkController {
        id: linkController
        viewModel: network
    }

    Keys.onTabPressed: event => {
        tabMenu.x = root.cursorX;
        tabMenu.y = root.cursorY;
        tabMenu.open();
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace)
            network.deleteSelected();
        else if (event.key === Qt.Key_Escape && linkController.linking)
            linkController.cancel();
    }

    // Pan, zoom, and port interaction. A press near a port grabs the closest one
    // across every node, so the nearest port always wins over the topmost.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.MiddleButton | Qt.LeftButton
        hoverEnabled: true

        // True while a left drag is pulling a link out of a grabbed port.
        property bool draggingLink: false

        // True when the press grabbed a port, so the matching release is not also
        // read as a click that would finish the link.
        property bool grabbedOnPress: false

        onPressed: mouse => {
            root.mouseLastX = mouse.x;
            root.mouseLastY = mouse.y;
            grabbedOnPress = false;
            if (mouse.button !== Qt.LeftButton)
                return;

            const canvasPoint = Qt.point(root.toCanvasX(mouse.x), root.toCanvasY(mouse.y));
            const port = network.nodes.getGrabPort(canvasPoint);
            if (port.opId === undefined)
                return;

            linkController.grab(port.opId, port.slot, port.isOutput, Qt.point(port.x, port.y));
            grabbedOnPress = true;
            draggingLink = linkController.linking;
        }

        // A left click commits a snapped link, drops an unsnapped one, or clears
        // the selection. The press that grabbed a port does none of these, and a
        // click on a node body is consumed by the node.
        onClicked: mouse => {
            if (mouse.button !== Qt.LeftButton || grabbedOnPress)
                return;
            if (linkController.linking)
                linkController.finish();
            else
                network.clearSelection();
        }

        onReleased: {
            if (draggingLink) {
                linkController.release();
                draggingLink = false;
            }
        }

        onPositionChanged: mouse => {
            root.cursorX = mouse.x;
            root.cursorY = mouse.y;
            const canvasPoint = Qt.point(root.toCanvasX(mouse.x), root.toCanvasY(mouse.y));

            // A held drag pulls the link, a click placed link trails the cursor.
            if (draggingLink)
                linkController.drag(canvasPoint);
            else if (linkController.linking)
                linkController.update(canvasPoint);

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
        property color dotColor: Theme.network.dotColor
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
        id: canvasItem
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

        // Committed links render under the nodes so a curve never paints over a card.
        NodeLinkLayer {
            nodes: network.nodes
            links: network.edges
            linkColor: Theme.nodeLink.inactiveColor
        }

        Repeater {
            model: network.nodes

            delegate: Node {
                id: nodeDelegate
                viewZoom: root.viewZoom

                modelX: model.x
                modelY: model.y

                // True while this node anchors either end of the link being dragged.
                readonly property bool linkEndpoint: linkController.linking && (model.opId === linkController.originOpId || model.opId === linkController.hoverOpId)

                // An endpoint node rises above the floating layer, so the link tucks
                // under its ports while still drawing over the nodes it crosses.
                z: linkEndpoint ? 2 : 0
                label: model.name
                selected: model.selected
                primary: model.primary
                display: model.display
                inputSlotCount: model.inputSlotCount
                outputSlotCount: model.outputSlotCount
                linking: linkController.linking

                // The highlighted port when it is one of this node's own.
                readonly property var highlight: root.highlightedPort && root.highlightedPort.opId === model.opId ? root.highlightedPort : null
                highlightedInputSlot: highlight && !highlight.isOutput ? highlight.slot : -1
                highlightedOutputSlot: highlight && highlight.isOutput ? highlight.slot : -1

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
                onDisplayToggled: network.setDisplayNode(model.opId)
            }
        }

        // The in-progress link renders above the nodes so it is never hidden behind
        // a card. It reuses the link layer so it shares every link feature and style.
        NodeLinkLayer {
            z: 1
            floatingActive: linkController.linking
            floatingOutput: linkController.outputPoint
            floatingInput: linkController.inputPoint
            linkColor: Theme.var.accent
        }
    }
}
