import QtQuick
import QtQuick.Controls
import Enzo

// Themed tooltip shared by every hoverable control.
//
// e.g.
//   Tooltip {
//       text: "Delete point"
//       visible: mouse.containsMouse
//   }
ToolTip {
    id: root

    delay: Theme.tooltip.delay

    background: Rectangle {
        radius: 6
        color: Theme.tooltip.backgroundColor
        border.color: Theme.tooltip.borderColor
    }

    contentItem: Text {
        text: root.text
        color: Theme.tooltip.textColor
        font.family: Theme.var.fontSans
        font.pixelSize: 12
        font.weight: Font.Medium
    }
}
