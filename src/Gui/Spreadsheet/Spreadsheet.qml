import QtQuick

// Geometry spreadsheet: a primitives tree sidebar on the left and the attribute
// table of the selected primitive on the right.
Item {
    id: root

    required property var viewModel

    PrimitivesTree {
        id: tree

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 252
        model: root.viewModel.primitiveTree
        onPrimitiveSelected: index => root.viewModel.selectPrimitive(index)
    }

    AttributeTable {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: tree.right
        anchors.right: parent.right
        anchors.leftMargin: 7
        model: root.viewModel.tableModel
    }
}
