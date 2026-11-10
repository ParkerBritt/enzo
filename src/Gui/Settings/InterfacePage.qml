import QtQuick
import Enzo
import "../Components"

// The page that sets how the interface is drawn.
Item {
    id: root

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 11
        spacing: 12

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "UI Scale"
            color: Theme.var.text
            font.family: Theme.var.fontSans
            font.pixelSize: 12
            font.weight: Font.Medium
        }

        Dropdown {
            labels: interfaceSettings.scales.map(scale => Math.round(scale * 100) + "%")
            currentIndex: interfaceSettings.scaleIndex
            onActivated: index => interfaceSettings.scaleIndex = index
        }
    }
}
