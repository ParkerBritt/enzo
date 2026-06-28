import QtQuick
import "../Components"

// Floating panel showing the primary node's parameters.
Rectangle {
    id: panel

    // Layout constants.
    readonly property real defaultWidth: 500
    readonly property real defaultHeight: Theme.parameterPanelHeight
    readonly property real minWidth: 200
    readonly property real minHeight: 120
    readonly property real contentMargin: 12
    readonly property real maxHeightInset: 28
    readonly property real gripSize: 18

    width: defaultWidth
    visible: parameters.hasNode
    clip: true
    radius: Theme.panelRadius
    color: Qt.rgba(0.07, 0.07, 0.09, 0.93)
    border.color: Theme.border
    border.width: 1

    // Height follows the content until the user drags the resize grip, which
    // assigns an explicit size and takes over.
    implicitHeight: Math.max(defaultHeight, layout.implicitHeight + contentMargin * 2)
    height: Math.min(implicitHeight, parent ? parent.height - maxHeightInset : implicitHeight)

    Column {
        id: layout

        anchors.fill: parent
        anchors.margins: panel.contentMargin
        spacing: 8

        // Header naming the node the parameters belong to.
        Row {
            width: parent.width
            spacing: 8

            Icon {
                name: "sliders-horizontal"
                size: 15
                color: Theme.accent2
                anchors.verticalCenter: parent.verticalCenter
            }

            Column {
                Text {
                    text: parameters.nodeType
                    color: Theme.label
                    font.family: Theme.fontUi
                    font.pixelSize: 10
                }
                Text {
                    text: parameters.nodeName
                    color: Theme.name
                    font.family: Theme.fontUi
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.bsoft
        }

        ParameterList {
            width: parent.width
            model: parameters.parameters
        }
    }

    // Drag the bottom left corner to resize. The top right stays pinned, so the
    // panel grows left and down.
    MouseArea {
        id: grip

        width: panel.gripSize
        height: panel.gripSize
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeBDiagCursor

        property point origin
        property real startWidth
        property real startHeight

        onPressed: mouse => {
            origin = mapToItem(null, mouse.x, mouse.y);
            startWidth = panel.width;
            startHeight = panel.height;
        }
        onPositionChanged: mouse => {
            if (!pressed)
                return;
            let here = mapToItem(null, mouse.x, mouse.y);
            panel.width = Math.max(panel.minWidth, startWidth - (here.x - origin.x));
            panel.height = Math.max(panel.minHeight, startHeight + (here.y - origin.y));
        }
    }
}
