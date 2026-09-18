import QtQuick
import QtQuick.Controls
import Enzo
import "../Components"
import "."

Rectangle {
    id: root

    color: Theme.var.surface
    radius: Theme.var.panelRadius
    border.color: Theme.var.border
    focus: true
    clip: true

    property real linkHitRadius: 20

    LinkRedirectController {
        id: redirectController
        links: committedLinks
        linkDrag: linkController
        hitRadius: root.linkHitRadius
        enabled: !cutController.held
    }

    LinkCutController {
        id: cutController
        links: committedLinks
        hitRadius: root.linkHitRadius
    }

    NodeDropController {
        id: dropController
        links: committedLinks
        insertRadius: network.nodeHeight
    }

    NetworkViewTransform {
        id: view
        panX: root.width / 2
        panY: root.height / 2
    }

    NodeLinkController {
        id: linkController
        viewModel: network
    }

    NetworkKeys {
        id: keyBindings
        nodeDrop: dropController
        cutter: cutController
        linkDrag: linkController
        onMenuRequested: {
            tabMenu.x = mouseInput.cursorX;
            tabMenu.y = mouseInput.cursorY;
            tabMenu.open();
        }
    }

    // Focus belongs to this item, so its key events go to the bindings.
    Keys.forwardTo: [keyBindings]

    // Clears the held keys when focus moves away, since their release goes elsewhere.
    onActiveFocusChanged: if (!activeFocus) keyBindings.clearHeld()

    FocusReclaimer {
        target: root
        area: mouseInput
    }

    NetworkMouse {
        id: mouseInput
        viewTransform: view
        linkDrag: linkController
        cutter: cutController
        redirect: redirectController
        selection: selectionBox
    }

    NetworkBackground {
        anchors.fill: parent
        zoom: view.zoom
        pan: Qt.point(view.panX, view.panY)
    }

    NetworkEmptyHint {
        anchors.centerIn: parent
        visible: nodeRepeater.count === 0
    }

    TabMenu {
        id: tabMenu
        nodeTypes: network.nodeTypes
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onNodeTypeChosen: (name, chainToPrimary) => {
            if (chainToPrimary && network.chainNodeToPrimary(name))
                return;
            network.createNode(name, view.toCanvasX(x), view.toCanvasY(y));
        }
    }

    Item {
        id: canvasItem
        transform: [
            Scale {
                xScale: view.zoom
                yScale: view.zoom
            },
            Translate {
                x: view.panX
                y: view.panY
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
            previewColor: Theme.nodeLink.activeColor
            previewCutLinks: dropController.preview ? dropController.preview.cutLinks : []
            previewLinks: dropController.preview ? dropController.preview.newLinks : []
        }

        NetworkNodes {
            id: nodeRepeater
            viewZoom: view.zoom
            linkDrag: linkController
            nodeDrop: dropController
            cursorPoint: mouseInput.cursorPoint
        }

        // The in-progress link renders above the nodes so a card never hides it.
        NodeLinkLayer {
            z: 1
            floatingActive: linkController.linking
            floatingOutput: linkController.outputPoint
            floatingInput: linkController.inputPoint
            linkColor: Theme.nodeLink.activeColor
        }

        // The selection box draws over every node it covers.
        SelectionBox {
            id: selectionBox
            z: 3
            viewZoom: view.zoom
        }
    }

    IconCursor {
        anchors.fill: parent
        active: cutController.held
        name: "slice"
        color: cutController.cutting ? Theme.nodeLink.cutColor : Theme.var.text
        size: cutController.cutting ? 20 : 22
        hotSpot: Qt.point(2, 20)
    }
}
