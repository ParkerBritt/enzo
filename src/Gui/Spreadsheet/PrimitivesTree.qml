import QtQuick
import QtQuick.Controls
import "../Components"

// Sidebar listing the primitives of the selected node as a path tree. Tapping a
// primitive leaf asks the view-model to show it in the attribute table.
Rectangle {
    id: root

    property var viewModel
    property int selectedIndex: 0

    color: Theme.panelHeader

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.bsoft
    }

    // Section title and primitive count.
    Item {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 32

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.bsoft
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: "PRIMITIVES"
            color: Theme.muted
            font.family: Theme.fontUi
            font.pixelSize: 10
            font.weight: Font.Bold
            font.letterSpacing: 1
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            Text {
                text: root.viewModel.primitiveCount
                color: Theme.name
                font.family: Theme.fontMono
                font.pixelSize: 11
            }
            Text {
                text: "prims"
                color: Theme.label
                font.family: Theme.fontMono
                font.pixelSize: 11
            }
        }
    }

    TreeView {
        id: tree

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 7
        clip: true

        model: root.viewModel.primitiveTree

        delegate: Item {
            id: node

            required property TreeView treeView
            required property bool hasChildren
            required property bool expanded
            required property int depth
            required property int row
            required property string name
            required property string typeTag
            required property int primitiveIndex

            readonly property bool isLeaf: primitiveIndex >= 0
            readonly property bool selected: isLeaf && primitiveIndex === root.selectedIndex
            readonly property string tag: isLeaf ? typeTag : "GROUP"

            implicitWidth: tree.width
            implicitHeight: 27

            // Vertical guide lines for each ancestor depth.
            Repeater {
                model: node.depth

                delegate: Rectangle {
                    required property int index
                    x: 9 + index * 14 + 7
                    y: -2
                    width: 1
                    height: node.height + 4
                    color: Qt.rgba(1, 1, 1, 0.135 - Math.min(index, 6) * 0.012)
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: 6
                color: node.selected ? Qt.rgba(0.545, 0.361, 0.965, 0.13) : "transparent"
                border.color: node.selected ? Qt.rgba(0.545, 0.361, 0.965, 0.32) : "transparent"
            }

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 9 + node.depth * 14
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 7

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron-down"
                    size: 10
                    color: "#7a7a86"
                    visible: node.hasChildren
                    rotation: node.expanded ? 0 : -90
                }
                Item {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    height: 10
                    visible: !node.hasChildren
                }

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: node.isLeaf ? (node.typeTag === "CAMERA" ? "camera" : "box") : "layers"
                    size: 15
                    color: node.selected ? Theme.accent2 : "#aab0ba"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: node.name
                    color: node.selected ? Theme.name : "#d2d3d9"
                    font.family: Theme.fontUi
                    font.pixelSize: 12
                }
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 52
                horizontalAlignment: Text.AlignRight
                text: node.tag
                color: node.selected ? "#b9a3f5" : "#6c6c76"
                font.family: Theme.fontMono
                font.pixelSize: 8
                font.weight: Font.Bold
                font.letterSpacing: 0.6
            }

            TapHandler {
                onTapped: {
                    if (node.hasChildren)
                        node.treeView.toggleExpanded(node.row)
                    if (node.isLeaf) {
                        root.selectedIndex = node.primitiveIndex
                        root.viewModel.selectPrimitive(node.primitiveIndex)
                    }
                }
            }
        }
    }
}
