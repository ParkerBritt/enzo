import QtQuick
import Enzo
import "../Components"

// The centred notice a settings page shows while the feature behind it is unbuilt.
Item {
    id: root

    property string icon: ""
    property string heading: ""
    property string body: ""

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 320)
        spacing: 12

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 44
            height: 44
            radius: 13
            color: Theme.var.surfaceRaised
            border.color: Theme.var.border

            Icon {
                anchors.centerIn: parent
                name: root.icon
                size: 20
                color: Theme.var.textLabel
            }
        }

        Text {
            width: parent.width
            text: root.heading
            color: Theme.var.text
            font.family: Theme.var.fontSans
            font.pixelSize: 13
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            text: root.body
            color: Theme.var.textLabel
            font.family: Theme.var.fontSans
            font.pixelSize: 12
            lineHeight: 18
            lineHeightMode: Text.FixedHeight
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
