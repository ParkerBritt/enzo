import QtQuick
import Enzo
import "../Components"

// A labelled section break between runs of parameters.
Item {
    id: divider

    required property var item

    property int rowIndex: 0

    readonly property string iconName: item ? item.icon : ""
    readonly property string label: item ? item.label : ""

    // Extra space above the rule so a new section reads as a break from the one above.
    readonly property int topPadding: rowIndex === 0 ? 0 : 10

    implicitHeight: 24 + topPadding

    Icon {
        id: leadIcon

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: divider.topPadding / 2
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
        anchors.verticalCenterOffset: divider.topPadding / 2
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
        anchors.verticalCenterOffset: divider.topPadding / 2
        height: 1
        color: Theme.var.border
    }
}
