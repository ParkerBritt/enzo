import QtQuick
import Enzo
import "../Style"

Rectangle {
    id: parameter

    required property var item
    implicitHeight: Constants.parameterHeight
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
        onAccepted: focus = false
        Keys.onEscapePressed: {
            text = parameter.item ? parameter.item.value : "";
            focus = false;
        }
    }
}
