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

    // How near the cursor must fall to a link to cut, pick up, or hover it.
    property real linkHitRadius: 20

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

    // Removes a link, dissolving it outward from the cut point.
    function cutLink(linkIndex, canvasPoint) {
        if (linkIndex < 0)
            return;
        committedLinks.fadeLink(linkIndex, canvasPoint);
        network.removeLink(linkIndex);
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

        // True while the cursor rests on a link a press would pick up.
        property bool overRedirect: false
        cursorShape: draggingLink ? Qt.ClosedHandCursor
                     : overRedirect ? Qt.PointingHandCursor
                     : Qt.ArrowCursor

        // True while a left drag is pulling a link out of a grabbed port.
        property bool draggingLink: false

        // True when the press grabbed a port, so the matching release is not also
        // read as a click that would finish the link.
        property bool grabbedOnPress: false

        // True while a Ctrl drag is cutting across links.
        property bool cutting: false
        // The last canvas point a cutting drag passed through.
        property point cutLast

        onPressed: mouse => {
            root.mouseLastX = mouse.x;
            root.mouseLastY = mouse.y;
            grabbedOnPress = false;
            cutting = false;
            if (mouse.button !== Qt.LeftButton)
                return;

            const canvasPoint = Qt.point(root.toCanvasX(mouse.x), root.toCanvasY(mouse.y));

            // Ctrl turns the left button into a link cutter.
            if (mouse.modifiers & Qt.ControlModifier) {
                cutting = true;
                cutLast = canvasPoint;
                return;
            }

            const port = network.nodes.getGrabPort(canvasPoint);
            if (port.opId !== undefined) {
                linkController.grab(port.opId, port.slot, port.isOutput, Qt.point(port.x, port.y));
                grabbedOnPress = true;
                draggingLink = linkController.linking;
                return;
            }

            // Away from every port, a press on a link picks it up by its nearer end.
            if (!linkController.linking)
                pickUpLink(canvasPoint);
        }

        // Detaches the pressed end of the link under the cursor and hands it to
        // the link controller so the drag can rewire it onto another port.
        function pickUpLink(canvasPoint) {
            const hit = committedLinks.linkAt(canvasPoint, root.linkHitRadius);
            if (hit.linkIndex < 0)
                return;

            const ends = network.getLinkEndpoints(hit.linkIndex);
            network.removeLink(hit.linkIndex);
            const anchor = Qt.point(hit.anchorX, hit.anchorY);
            if (hit.atOutputEnd)
                linkController.grab(ends.targetOp, ends.targetInput, false, anchor);
            else
                linkController.grab(ends.sourceOp, ends.sourceOutput, true, anchor);
            linkController.drag(canvasPoint);
            grabbedOnPress = true;
            draggingLink = true;
        }

        // A left click commits a snapped link, drops an unsnapped one, or clears
        // the selection. The press that grabbed a port does none of these, and a
        // click on a node body is consumed by the node.
        onClicked: mouse => {
            if (mouse.button !== Qt.LeftButton || grabbedOnPress)
                return;

            // A Ctrl click cuts the link under the cursor.
            if (mouse.modifiers & Qt.ControlModifier) {
                const canvasPoint = Qt.point(root.toCanvasX(mouse.x), root.toCanvasY(mouse.y));
                root.cutLink(committedLinks.linkAt(canvasPoint, root.linkHitRadius).linkIndex, canvasPoint);
                committedLinks.setHover(-1, NodeLinkLayer.None);
                return;
            }

            if (linkController.linking)
                linkController.finish();
            else
                network.clearSelection();
        }

        onReleased: {
            cutting = false;
            if (draggingLink) {
                linkController.release();
                draggingLink = false;
            }
        }

        onExited: {
            committedLinks.setHover(-1, NodeLinkLayer.None);
            overRedirect = false;
        }

        onPositionChanged: mouse => {
            root.forceActiveFocus();
            root.cursorX = mouse.x;
            root.cursorY = mouse.y;
            const canvasPoint = Qt.point(root.toCanvasX(mouse.x), root.toCanvasY(mouse.y));

            // A Ctrl drag cuts every link its path sweeps across.
            if (cutting) {
                root.cutLink(committedLinks.linkCrossing(cutLast, canvasPoint), canvasPoint);
                cutLast = canvasPoint;
            }

            // The hover preview mirrors what a press at this point would do.
            overRedirect = false;
            if (mouse.modifiers & Qt.ControlModifier) {
                committedLinks.setHover(committedLinks.linkAt(canvasPoint, root.linkHitRadius).linkIndex, NodeLinkLayer.Cut);
            } else if (draggingLink || linkController.linking || network.nodes.isOverNodeBody(canvasPoint) || network.nodes.getGrabPort(canvasPoint).opId !== undefined) {
                committedLinks.setHover(-1, NodeLinkLayer.None);
            } else {
                const hit = committedLinks.linkAt(canvasPoint, root.linkHitRadius);
                committedLinks.setHover(hit.linkIndex, NodeLinkLayer.Redirect, hit.atOutputEnd === true);
                overRedirect = hit.linkIndex >= 0;
            }

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
            id: committedLinks
            nodes: network.nodes
            links: network.edges
            linkColor: Theme.nodeLink.inactiveColor
            cutColor: Theme.nodeLink.cutColor
            redirectColor: Theme.nodeLink.redirectColor
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
            linkColor: Theme.nodeLink.activeColor
        }
    }
}
