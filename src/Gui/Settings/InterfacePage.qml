import QtQuick
import Enzo
import "../Components"

// The page that sets how the interface is drawn.
Item {
    id: root

    Grid {
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 11
        columns: 2
        columnSpacing: 12
        rowSpacing: 10
        verticalItemAlignment: Grid.AlignVCenter

        SettingLabel {
            text: "UI Scale"
        }

        Dropdown {
            labels: interfaceSettings.scales.map(scale => Math.round(scale * 100) + "%")
            currentIndex: interfaceSettings.scaleIndex
            onActivated: index => interfaceSettings.scaleIndex = index
        }

        SettingLabel {
            text: "Animation Speed"
        }

        Dropdown {
            labels: interfaceSettings.animationSpeedLabels
            currentIndex: interfaceSettings.animationSpeedIndex
            onActivated: index => interfaceSettings.animationSpeedIndex = index
        }
    }

    component SettingLabel: Text {
        color: Theme.var.text
        font.family: Theme.var.fontSans
        font.pixelSize: 12
        font.weight: Font.Medium
    }
}
