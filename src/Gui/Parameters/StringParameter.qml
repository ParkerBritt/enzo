import QtQuick
import Enzo

Rectangle {
    id: parameter

    required property var item
    implicitHeight: 22
    radius: Theme.parameter.borderRadius
    color: Theme.parameter.backgroundColor
    border.color: Theme.parameter.lineColor

    TextInput {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        verticalAlignment: TextInput.AlignVCenter
        clip: true
        text: parameter.item ? parameter.item.value : ""
        color: Theme.var.text
        font.family: Theme.var.fontSans
        font.pixelSize: 12
        onEditingFinished: if (parameter.item) parameter.item.value = text
    }
}
