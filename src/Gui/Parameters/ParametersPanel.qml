import QtQuick
import QtQuick.Effects
import Enzo
import "../Components"

// Floating panel showing the primary node's parameters.
Item {
    id: panel

    // Layout constants.
    readonly property real defaultWidth: 500
    readonly property real defaultHeight: Theme.parameter.panelHeight
    readonly property real minWidth: 200
    readonly property real minHeight: 120
    // Space above and below the panel's content.
    readonly property real verticalMargin: 12
    // Inset between the panel edge and its content.
    readonly property real sideMargin: 16
    // Extra left padding inside each parameter row. Not all parameters will use this.
    readonly property real parameterInset: 20
    readonly property real maxHeightInset: 28
    readonly property real gripSize: 18
    readonly property real radius: Theme.var.panelRadius

    width: defaultWidth
    visible: parameters.hasNode

    // Height follows the content until the user drags the resize grip, which
    // assigns an explicit size and takes over.
    implicitHeight: Math.max(defaultHeight, header.height + 1 + list.implicitHeight + verticalMargin)
    height: Math.min(implicitHeight, parent ? parent.height - maxHeightInset : implicitHeight)

    // Lifts the panel off the network behind it, so its edges stay readable
    // against a bright canvas.
    Rectangle {
        id: shadowCaster
        anchors.fill: parent
        radius: panel.radius
        color: Theme.parameter.panelColor
    }

    MultiEffect {
        source: shadowCaster
        anchors.fill: shadowCaster
        shadowEnabled: true
        shadowBlur: 1.0
        shadowOpacity: 0.55
        shadowVerticalOffset: 6
    }

    // The panel body, clipped to the rounded outline.
    Rectangle {
        id: body

        anchors.fill: parent
        clip: true
        radius: panel.radius
        color: Theme.parameter.panelColor

        // Gives the panel keyboard focus on hover, without blocking clicks to its
        // controls or stealing focus from a field being edited.
        MouseArea {
            id: panelHover

            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
        }

        FocusReclaimer {
            target: panel
            area: panelHover
        }

        // Title strip naming the node the parameters belong to.
        Rectangle {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: headerRow.implicitHeight + panel.verticalMargin * 2
            topLeftRadius: panel.radius
            topRightRadius: panel.radius
            color: Theme.var.surfaceRaised

            Row {
                id: headerRow
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: panel.sideMargin
                spacing: 8

                Icon {
                    name: "sliders-horizontal"
                    size: 15
                    color: Theme.var.accentBright
                    anchors.verticalCenter: parent.verticalCenter
                }

                Column {
                    Text {
                        text: parameters.nodeType
                        color: Theme.var.textLabel
                        font.family: Theme.var.fontSans
                        font.pixelSize: 10
                    }
                    Text {
                        text: parameters.nodeName
                        color: Theme.var.textStrong
                        font.family: Theme.var.fontSans
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }
                }
            }
        }

        Rectangle {
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.var.borderSoft
        }

        ParameterList {
            id: list
            anchors.top: header.bottom
            anchors.topMargin: panel.verticalMargin
            anchors.left: parent.left
            anchors.leftMargin: panel.sideMargin
            anchors.right: parent.right
            anchors.rightMargin: panel.sideMargin
            contentInset: panel.parameterInset
            model: parameters.parameters
        }

        // Drawn last so the border isn't painted over by the header fill.
        Rectangle {
            anchors.fill: parent
            radius: panel.radius
            color: "transparent"
            border.color: Theme.var.borderSoft
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
