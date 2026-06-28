import QtQuick

// Geometry spreadsheet drawer: a header bar over a primitives tree sidebar and
// the attribute table of the selected primitive.
Rectangle {
    id: root

    required property var viewModel

    // Layout constants.
    readonly property real headerHeight: 46
    readonly property real sidebarWidth: 252

    color: Theme.panel
    radius: Theme.panelRadius
    border.color: Theme.border
    clip: true

    SpreadsheetHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.headerHeight
        viewModel: root.viewModel
    }

    PrimitivesTree {
        id: sidebar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: root.sidebarWidth
        viewModel: root.viewModel
    }

    AttributeTable {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: sidebar.right
        anchors.right: parent.right
        viewModel: root.viewModel
    }
}
