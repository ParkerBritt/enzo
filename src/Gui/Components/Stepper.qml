import QtQuick
import Enzo

// Pill walking a selection through a sequence one step at a time.
Rectangle {
    id: root

    property string label

    // Dimmer trailing text, such as the total the label counts against.
    property string detail

    signal previous
    signal next

    implicitWidth: row.implicitWidth + 4
    implicitHeight: 30
    radius: Theme.parameter.borderRadius
    color: Theme.parameter.backgroundColor
    border.color: Theme.parameter.lineColor

    Row {
        id: row

        anchors.centerIn: parent

        IconButton {
            name: "chevron-left"
            width: 17
            height: 26
            iconSize: 12
            onClicked: root.previous()
        }

        Item {
            width: Math.max(34, labelRow.implicitWidth + 10)
            height: 26

            Row {
                id: labelRow

                anchors.centerIn: parent

                Text {
                    text: root.label
                    color: Theme.var.text
                    font.family: Theme.var.fontSans
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
                Text {
                    text: root.detail
                    color: Theme.var.textMuted
                    font.family: Theme.var.fontSans
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }
        }

        IconButton {
            name: "chevron-right"
            width: 17
            height: 26
            iconSize: 12
            onClicked: root.next()
        }
    }
}
