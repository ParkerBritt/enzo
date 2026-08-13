import QtQuick
import Enzo

// One icon button in a ParameterContextMenu, with its label underneath. The
// hover background covers the icon and its label as a single block.
//
// e.g.
//   ParameterContextMenuAction {
//       iconName: "copy"
//       label: "Copy"
//       onTriggered: clip.copy(...)
//   }
Rectangle {
    id: root

    property string iconName: ""
    property string label: ""
    property string tooltip: ""
    property bool enabled: true

    signal triggered

    readonly property color contentColor: root.enabled ? Theme.var.textLabel : Theme.var.textFaint

    implicitWidth: content.implicitWidth + 16
    implicitHeight: content.implicitHeight + 10
    radius: Theme.parameter.borderRadius
    color: mouse.containsMouse && root.enabled ? Theme.iconButton.plainHoverColor : "transparent"
    opacity: root.enabled ? 1 : 0.35
    scale: mouse.pressed && root.enabled ? 0.88 : (mouse.containsMouse && root.enabled ? 1.06 : 1)

    Behavior on scale {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutCubic
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: 120
            easing.type: Easing.OutCubic
        }
    }

    Column {
        id: content

        anchors.centerIn: parent
        spacing: 4

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: root.iconName
            size: 15
            color: root.contentColor
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.family: Theme.var.fontSans
            font.pixelSize: 10
            color: root.contentColor
        }
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        onClicked: root.triggered()
    }

    Tooltip {
        text: root.tooltip
        visible: root.tooltip !== "" && mouse.containsMouse
    }
}
