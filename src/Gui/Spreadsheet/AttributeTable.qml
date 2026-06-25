import QtQuick
import QtQuick.Controls
import "../Components"

// Attribute table of the selected primitive. Column headers and values are
// tinted by axis, the active row carries a mode coloured bar, and a footer
// reports the element count and cook stats.
Item {
    id: root

    property var viewModel
    property int selectedRow: 0

    readonly property string modeColor: Theme.modeColors[root.viewModel.mode]
    readonly property var modeIcons: ["grip", "spline", "triangle", "pentagon"]

    function axisColor(axis, light) {
        if (axis === 0) return light ? Theme.axisXLight : Theme.axisX;
        if (axis === 1) return light ? Theme.axisYLight : Theme.axisY;
        if (axis === 2) return light ? Theme.axisZLight : Theme.axisZ;
        return "#8a8b94";
    }
    function headerAxis(text) {
        switch (text.slice(-1)) {
        case "x": case "r": case "u": return 0;
        case "y": case "g": case "v": return 1;
        case "z": case "b": case "w": return 2;
        default: return -1;
        }
    }

    // Column header row, with an index corner on the left.
    Rectangle {
        id: columnHeader

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 32
        color: Theme.panelHeader

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.bsoft
        }

        Text {
            width: 42
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignRight
            rightPadding: 14
            text: "#"
            color: "#5a5a64"
            font.family: Theme.fontMono
            font.pixelSize: 10
            font.weight: Font.DemiBold
        }

        HorizontalHeaderView {
            id: horizontalHeader

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 42
            syncView: table
            clip: true

            delegate: Item {
                implicitHeight: 32

                Text {
                    anchors.fill: parent
                    rightPadding: 14
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    text: display
                    color: root.axisColor(root.headerAxis(display), false)
                    font.family: Theme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.4
                }
            }
        }
    }

    // Row index gutter, synced with the table scroll.
    VerticalHeaderView {
        id: rowHeader

        anchors.top: columnHeader.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        width: 42
        syncView: table
        clip: true

        delegate: Item {
            implicitWidth: 42

            Text {
                anchors.fill: parent
                rightPadding: 14
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                text: display
                color: "#5a5a64"
                font.family: Theme.fontMono
                font.pixelSize: 12
            }
        }
    }

    TableView {
        id: table

        anchors.top: columnHeader.bottom
        anchors.left: rowHeader.right
        anchors.right: parent.right
        anchors.bottom: footer.top
        clip: true

        model: root.viewModel.tableModel
        columnWidthProvider: () => 86
        rowHeightProvider: () => 30

        delegate: Item {
            id: cell

            required property string display
            required property int axis
            required property int row
            required property int column

            readonly property bool selected: row === root.selectedRow

            Rectangle {
                anchors.fill: parent
                color: cell.selected ? ("#1f" + root.modeColor.slice(1)) : "transparent"
            }

            Rectangle {
                visible: cell.selected && cell.column === 0
                width: 2
                height: parent.height
                color: root.modeColor
            }

            Text {
                anchors.fill: parent
                rightPadding: 14
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                text: cell.display
                color: root.axisColor(cell.axis, true)
                font.family: Theme.fontMono
                font.pixelSize: 12
            }

            TapHandler {
                onTapped: root.selectedRow = cell.row
            }
        }
    }

    // Footer with the active count and cook stats.
    Rectangle {
        id: footer

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
        color: Theme.panelHeader

        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 1
            color: Theme.bsoft
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 13
            anchors.verticalCenter: parent.verticalCenter
            spacing: 7

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: root.modeIcons[root.viewModel.mode]
                size: 13
                color: root.modeColor
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.viewModel.elementCount
                color: Theme.name
                font.family: Theme.fontMono
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.viewModel.elementNoun
                color: Theme.muted
                font.family: Theme.fontMono
                font.pixelSize: 11
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 13
            anchors.verticalCenter: parent.verticalCenter
            spacing: 14

            Repeater {
                model: [
                    { label: "MEMORY", value: "1.42 MB", color: "#cfd0d6" },
                    { label: "COOK", value: "3.41 ms", color: "#7fd3a6" }
                ]

                delegate: Row {
                    id: stat

                    required property var modelData
                    spacing: 6

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: stat.modelData.label
                        color: "#5a5a64"
                        font.family: Theme.fontUi
                        font.pixelSize: 8
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.5
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: stat.modelData.value
                        color: stat.modelData.color
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                    }
                }
            }
        }
    }
}
