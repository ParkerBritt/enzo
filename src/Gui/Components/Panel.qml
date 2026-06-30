import QtQuick
import QtQuick.Shapes
import Enzo

/// Rounds and clips panel contents.
Item {
    id: root

    property real radius: Theme.var.panelRadius
    property real borderWidth: 1
    property color borderColor: Theme.var.border

    // Place content inside container
    default property alias content: contentContainer.data

    Rectangle {
        anchors.fill: parent
        color: Theme.var.surface
        radius: root.radius
    }

    Item {
        id: contentContainer
        anchors.fill: parent
        anchors.margins: root.borderWidth
    }

    // Overlays content with rounded outline.
    Shape {
        id: frame
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        // Page background fills the corners, hiding content past the radius.
        ShapePath {
            fillColor: Theme.var.background
            strokeColor: "transparent"
            fillRule: ShapePath.OddEvenFill
            PathRectangle {
                width: frame.width
                height: frame.height
            }
            PathRectangle {
                width: frame.width
                height: frame.height
                radius: root.radius
            }
        }

        // Border tracing the rounded edge.
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.borderColor
            strokeWidth: root.borderWidth
            PathRectangle {
                x: root.borderWidth / 2
                y: root.borderWidth / 2
                width: frame.width - root.borderWidth
                height: frame.height - root.borderWidth
                radius: root.radius
            }
        }
    }
}
