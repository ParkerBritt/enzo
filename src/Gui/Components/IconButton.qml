import QtQuick
import Enzo

// Square icon button, styled plain, field, or accent by its variant.
Rectangle {
    id: root

    property string variant: "plain"
    property alias name: icon.name
    property alias iconSize: icon.size
    property color iconColor: Theme.var.textLabel
    property string tooltip: ""

    // Surfaces set by the variant states below.
    property color surfaceColor: "transparent"
    property color hoverColor: Theme.iconButton.plainHoverColor

    signal clicked()

    width: 30
    height: 30
    radius: Theme.parameter.borderRadius
    color: mouse.containsMouse ? root.hoverColor : root.surfaceColor
    border.color: root.variant === "field" ? Theme.parameter.lineColor : "transparent"
    opacity: enabled ? 1 : 0.35

    states: [
        State {
            name: "field"
            when: root.variant === "field"
            PropertyChanges {
                root.surfaceColor: Theme.iconButton.fieldColor
                root.hoverColor: Theme.iconButton.fieldHoverColor
            }
        },
        State {
            name: "accent"
            when: root.variant === "accent"
            PropertyChanges {
                root.surfaceColor: Theme.iconButton.accentColor
                root.hoverColor: Theme.iconButton.accentHoverColor
                root.iconColor: Theme.var.textStrong
            }
        }
    ]

    Icon {
        id: icon

        anchors.centerIn: parent
        size: 13
        color: root.iconColor
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    Tooltip {
        text: root.tooltip
        visible: root.tooltip !== "" && mouse.containsMouse
    }
}
