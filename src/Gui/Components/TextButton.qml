import QtQuick
import Enzo

// Text button, styled plain or accent by its variant.
Rectangle {
    id: root

    property string text: ""
    property string variant: "plain"

    signal clicked

    readonly property bool accent: root.variant === "accent"

    implicitWidth: label.implicitWidth + 28
    implicitHeight: 28
    radius: 8
    color: root.enabled ? (root.accent ? (mouse.containsMouse ? Theme.var.accentBright : Theme.var.accent) : (mouse.containsMouse ? Theme.var.selectedFill : Theme.var.surfaceRaised)) : Theme.var.surface
    border.color: root.enabled ? (root.accent ? color : Theme.var.fieldBorder) : Theme.var.borderSoft

    Text {
        id: label

        anchors.centerIn: parent
        text: root.text
        color: root.enabled ? (root.accent ? Theme.var.textStrong : Theme.var.text) : Theme.var.textMuted
        font.family: Theme.var.fontSans
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        onClicked: root.clicked()
    }
}
