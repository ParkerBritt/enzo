import QtQuick
import Enzo
import "../Components"

// A labelled section break between runs of parameters.
Item {
    id: divider

    required property var item

    readonly property string iconName: item ? item.icon : ""
    readonly property string label: item ? item.label : ""

    implicitHeight: 24

    Icon {
        id: leadIcon

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        visible: divider.iconName !== ""
        width: visible ? size : 0
        name: divider.iconName
        color: Theme.var.textLabel
        size: 13
    }

    Text {
        id: caption

        anchors.left: leadIcon.right
        anchors.leftMargin: leadIcon.visible ? 6 : 0
        anchors.verticalCenter: parent.verticalCenter
        text: divider.label
        color: Theme.var.textLabel
        font.family: Theme.var.fontSans
        font.pixelSize: 10
        font.weight: Font.Bold
        font.capitalization: Font.AllUppercase
    }

    Rectangle {
        anchors.left: caption.right
        anchors.leftMargin: caption.text !== "" ? 6 : 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: 1
        color: Theme.var.border
    }
}
