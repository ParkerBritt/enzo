import QtQuick
import QtQuick.Controls

// Sidebar listing the primitives of the selected node as a path tree. Tapping a
// primitive leaf asks the host to show it in the attribute table.
Rectangle {
    id: root

    property alias model: tree.model
    property int selectedIndex: -1
    signal primitiveSelected(int index)

    color: "#101015"
    border.color: "#262630"

    // Section title across the top of the sidebar.
    Text {
        id: title

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        height: 18
        text: "PRIMITIVES"
        color: "#9a9aa6"
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.letterSpacing: 1
    }

    TreeView {
        id: tree

        anchors.top: title.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 4
        clip: true

        delegate: Item {
            id: row

            required property TreeView treeView
            required property bool hasChildren
            required property bool expanded
            required property int depth
            required property int row
            required property string name
            required property string typeTag
            required property int primitiveIndex

            implicitWidth: tree.width
            implicitHeight: 27

            readonly property bool isSelected: primitiveIndex >= 0 && primitiveIndex === root.selectedIndex

            Rectangle {
                anchors.fill: parent
                color: row.isSelected ? "#8b5cf626" : "transparent"

                Rectangle {
                    width: 2
                    height: parent.height
                    color: "#8b5cf6"
                    visible: row.isSelected
                }
            }

            Row {
                anchors.fill: parent
                anchors.leftMargin: 10 + row.depth * 14
                anchors.rightMargin: 10
                spacing: 6

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    text: row.hasChildren ? (row.expanded ? "▾" : "▸") : ""
                    color: "#63636d"
                    font.pixelSize: 10
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.name
                    color: "#e7e8ec"
                    font.pixelSize: 12
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 10
                text: row.typeTag
                color: "#63636d"
                font.pixelSize: 8
                font.weight: Font.Bold
                font.letterSpacing: 0.5
                visible: row.typeTag.length > 0
            }

            TapHandler {
                onTapped: {
                    if (row.hasChildren)
                        row.treeView.toggleExpanded(row.row)
                    if (row.primitiveIndex >= 0) {
                        root.selectedIndex = row.primitiveIndex
                        root.primitiveSelected(row.primitiveIndex)
                    }
                }
            }
        }
    }
}
