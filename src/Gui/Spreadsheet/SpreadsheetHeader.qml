import QtQuick
import Enzo
import "../Components"

// Top bar of the spreadsheet drawer: title, breadcrumb path, component mode
// pill, and the find and collapse controls.
Rectangle {
    id: root

    property var viewModel
    readonly property var path: root.viewModel.nodePath

    color: Theme.var.surfaceHeader

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.var.borderSoft
    }

    // Title, breadcrumb.
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 13
        anchors.right: rightCluster.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "SPREADSHEET"
            color: Theme.var.textLabel
            font.family: Theme.var.fontSans
            font.pixelSize: 10
            font.weight: Font.Bold
            font.letterSpacing: 1.4
        }

        // Spacer
        Item {
            width: 7
            height: 1
        }

        NodePathLabel {
            anchors.verticalCenter: parent.verticalCenter
            path: root.path
        }
    }

    // Owner mode control, find button, and collaps button, on the right.
    Row {
        id: rightCluster

        anchors.right: parent.right
        anchors.rightMargin: 11
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        ModeControl {
            anchors.verticalCenter: parent.verticalCenter
            mode: root.viewModel.mode
            onModePicked: mode => root.viewModel.mode = mode
        }

        // TODO: implement search
        // Rectangle {
        //     id: searchButton
        //
        //     anchors.verticalCenter: parent.verticalCenter
        //     width: 27
        //     height: 27
        //     radius: 7
        //     color: "transparent"
        //     border.color: Theme.var.border
        //
        //     Icon {
        //         anchors.centerIn: parent
        //         name: "search"
        //         size: 14
        //         color: Theme.var.textLabel
        //     }
        //
        //     HoverHandler {
        //         id: searchHover
        //     }
        //
        //     Tooltip {
        //         text: "Search"
        //         visible: searchHover.hovered
        //     }
        // }

        // TODO: implement collapse
        // Rectangle {
        //     id: collapseButton
        //
        //     anchors.verticalCenter: parent.verticalCenter
        //     width: collapse.width + 22
        //     height: 27
        //     radius: 7
        //     color: Theme.var.accentDim
        //
        //     Row {
        //         id: collapse
        //
        //         anchors.centerIn: parent
        //         spacing: 7
        //
        //         Icon {
        //             anchors.verticalCenter: parent.verticalCenter
        //             name: "chevron-down"
        //             size: 13
        //             color: Theme.var.accentBright
        //         }
        //     }
        //
        //     HoverHandler {
        //         id: collapseHover
        //     }
        //
        //     Tooltip {
        //         text: "Collapse"
        //         visible: collapseHover.hovered
        //     }
        // }
    }
}
