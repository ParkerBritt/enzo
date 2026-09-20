import QtQuick
import Enzo

// Text button, styled plain, accent, danger or flat by its variant.
Rectangle {
    id: root

    property string text: ""
    property string variant: "plain"
    property alias icon: glyph.name

    signal clicked

    // The four colours a variant styles the button with.
    property color surfaceColor: Theme.var.surfaceRaised
    property color hoverColor: Theme.var.selectedFill
    property color borderColor: Theme.var.border
    property color labelColor: Theme.var.text

    implicitWidth: content.implicitWidth + 24
    implicitHeight: 28
    radius: 8
    color: mouse.containsMouse ? root.hoverColor : root.surfaceColor
    border.color: root.borderColor

    states: [
        State {
            name: "disabled"
            when: !root.enabled
            PropertyChanges {
                root.surfaceColor: Theme.var.surface
                root.hoverColor: Theme.var.surface
                root.borderColor: "transparent"
                root.labelColor: Theme.var.textMuted
            }
        },
        State {
            name: "accent"
            when: root.variant === "accent"
            PropertyChanges {
                root.surfaceColor: Theme.var.accent
                root.hoverColor: Theme.var.accentBright
                root.borderColor: root.color
                root.labelColor: Theme.var.textStrong
            }
        },
        State {
            name: "danger"
            when: root.variant === "danger"
            PropertyChanges {
                root.labelColor: Theme.var.danger
            }
        },
        State {
            name: "flat"
            when: root.variant === "flat"
            PropertyChanges {
                root.surfaceColor: "transparent"
                root.hoverColor: Theme.var.borderSoft
                root.borderColor: "transparent"
                root.labelColor: Theme.var.textLabel
            }
        }
    ]

    Row {
        id: content

        anchors.centerIn: parent
        spacing: 6

        Icon {
            id: glyph

            visible: glyph.name !== ""
            size: 13
            color: root.labelColor
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.text
            color: root.labelColor
            font.family: Theme.var.fontSans
            font.pixelSize: 12
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        onClicked: root.clicked()
    }
}
