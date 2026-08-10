import QtQuick
import Enzo
import "../Components"

// Segmented control switching the table between element classes. Each chip is
// an icon that brightens from gray to white when its class is active.
Rectangle {
    id: root

    property int mode: 0
    signal modePicked(int mode)

    readonly property var icons: ["../attributePoint", "../attributeVertex", "../attributeBase", "../attributePrimitive"]
    readonly property var labels: ["Points", "Vertices", "Faces", "Primitives"]

    implicitWidth: chips.width + 6
    implicitHeight: 27
    radius: 9
    color: Theme.spreadsheet.backgroundColor
    border.color: Theme.spreadsheet.lineColor

    Row {
        id: chips

        anchors.centerIn: parent
        spacing: 3

        Repeater {
            model: root.icons.length

            delegate: Rectangle {
                id: chip

                required property int index
                readonly property bool active: root.mode === index

                width: 30
                height: 21
                radius: 5
                color: active ? "#1fffffff" : "transparent"

                Icon {
                    anchors.centerIn: parent
                    name: root.icons[chip.index]
                    size: 15
                    color: chip.active ? "#ffffff" : "#75757e"
                }

                HoverHandler {
                    id: hover
                }

                TapHandler {
                    onTapped: root.modePicked(chip.index)
                }

                Tooltip {
                    text: root.labels[chip.index]
                    visible: hover.hovered
                }
            }
        }
    }
}
