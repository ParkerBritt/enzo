import QtQuick
import Enzo

// A group of parameters, stacked in a column or laid out in a row.
Column {
    id: group

    required property var item
    readonly property var members: item ? item.children : []
    readonly property bool horizontal: item && item.horizontal

    // Stacked members line up to one label column, shared with the panel.
    property real labelColumnWidth: stacked.implicitLabelWidth
    readonly property real implicitLabelWidth: horizontal ? 0 : stacked.implicitLabelWidth

    width: parent ? parent.width : 0
    spacing: 6

    Text {
        visible: group.item && !group.item.labelHidden
        text: group.item ? group.item.label : ""
        color: Theme.var.textLabel
        font.family: Theme.var.fontSans
        font.pixelSize: 10
        font.weight: Font.Bold
        font.capitalization: Font.AllUppercase
    }

    ParameterList {
        id: stacked
        visible: !group.horizontal
        width: group.width
        model: group.horizontal ? [] : group.members
        labelColumnWidth: group.labelColumnWidth
    }

    Row {
        id: side
        visible: group.horizontal
        width: group.width
        spacing: 8

        Repeater {
            id: cells
            model: group.horizontal ? group.members : []
            delegate: ParameterRow {
                required property var modelData
                item: modelData
                width: (side.width - side.spacing * (cells.count - 1)) / cells.count
            }
        }
    }
}
