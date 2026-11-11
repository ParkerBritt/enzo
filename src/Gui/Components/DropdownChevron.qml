import QtQuick
import Enzo

// The chevron on a control that opens a list, pointing up while the list is open.
Icon {
    id: root

    property bool open: false

    name: "chevron-down"
    size: 12
    color: Theme.var.textMuted
    rotation: root.open ? 180 : 0

    Behavior on rotation {
        NumberAnimation {
            duration: 150 * interfaceSettings.durationScale
            easing.type: Easing.OutCubic
        }
    }
}
