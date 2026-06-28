import QtQuick

Rectangle {
    id: field

    required property var item
    implicitHeight: 22
    radius: 7
    color: Theme.field
    border.color: Theme.fline

    TextInput {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        verticalAlignment: TextInput.AlignVCenter
        clip: true
        text: field.item.value
        color: Theme.text
        font.family: Theme.fontUi
        font.pixelSize: 12
        onEditingFinished: field.item.value = text
    }
}
