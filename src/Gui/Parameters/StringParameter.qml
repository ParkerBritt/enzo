import QtQuick

Rectangle {
    id: parameter

    required property var item
    implicitHeight: 22
    radius: Theme.parameterRadius
    color: Theme.parameterBg
    border.color: Theme.parameterLine

    TextInput {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        verticalAlignment: TextInput.AlignVCenter
        clip: true
        text: parameter.item ? parameter.item.value : ""
        color: Theme.text
        font.family: Theme.fontUi
        font.pixelSize: 12
        onEditingFinished: if (parameter.item) parameter.item.value = text
    }
}
