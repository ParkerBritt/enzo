import QtQuick
import "../Components"

// Segmented control switching the table between element classes. Each chip is
// an icon, tinted with its mode colour when active.
Rectangle {
    id: root

    property int mode: 0
    signal modePicked(int mode)

    readonly property var icons: ["grip", "spline", "triangle", "pentagon"]

    implicitWidth: chips.width + 6
    implicitHeight: 27
    radius: 9
    color: Theme.field
    border.color: Theme.fline

    Row {
        id: chips

        anchors.centerIn: parent
        spacing: 3

        Repeater {
            model: root.icons.length

            delegate: Rectangle {
                id: chip

                required property int index
                readonly property string accent: Theme.modeColors[index]
                readonly property bool active: root.mode === index

                width: 30
                height: 21
                radius: 5
                color: active ? ("#29" + accent.slice(1)) : "transparent"

                Icon {
                    anchors.centerIn: parent
                    name: root.icons[chip.index]
                    size: 15
                    color: chip.active ? chip.accent : "#74747e"
                }

                TapHandler {
                    onTapped: root.modePicked(chip.index)
                }
            }
        }
    }
}
