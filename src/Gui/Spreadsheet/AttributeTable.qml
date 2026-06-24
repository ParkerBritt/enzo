import QtQuick
import QtQuick.Controls

// Attribute table with a column header that scrolls in step with it. The table
// model is supplied by the host.
Item {
    id: root

    property alias model: table.model

    // Attribute names across the top, sized in step with the table below.
    HorizontalHeaderView {
        id: header

        anchors.top: parent.top
        anchors.left: table.left
        syncView: table
        clip: true

        delegate: Rectangle {
            implicitWidth: 96
            implicitHeight: 26
            color: "#191920"
            border.color: "#262630"
            Text {
                anchors.fill: parent
                anchors.leftMargin: 8
                verticalAlignment: Text.AlignVCenter
                text: display
                color: "#9a9aa6"
                font.pixelSize: 11
            }
        }
    }

    // One cell per attribute component, fed by the C++ attribute table model.
    TableView {
        id: table

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        delegate: Rectangle {
            implicitWidth: 96
            implicitHeight: 24
            color: "#141418"
            Text {
                anchors.fill: parent
                anchors.leftMargin: 8
                verticalAlignment: Text.AlignVCenter
                text: display
                color: "#e7e8ec"
                font.pixelSize: 12
            }
        }
    }
}
