import QtQuick

// A titled section holding its child parameters.
Column {
    id: group

    required property var item
    width: parent ? parent.width : 0
    spacing: 6

    Text {
        text: group.item.label
        color: Theme.label
        font.family: Theme.fontUi
        font.pixelSize: 10
        font.weight: Font.Bold
        font.capitalization: Font.AllUppercase
    }

    Repeater {
        model: group.item.children
        delegate: ParameterRow {
            required property var modelData
            item: modelData
            width: group.width
        }
    }
}
