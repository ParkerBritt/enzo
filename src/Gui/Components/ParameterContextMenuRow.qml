import QtQuick
import Enzo

// One row of a ParameterContextMenu or its paste submenu, shared by both so
// the icon, label and detail styling stay identical at every level. Sized to
// whichever PopupList is hosting it, read structurally off its own parent
// rather than a specific menu instance, since the same delegate renders rows
// at every nesting level.
//
// e.g.
//   ParameterContextMenuRow {
//       rowHeight: menu.rowHeight
//   }
Item {
    id: row

    required property int index
    required property var modelData
    required property int rowHeight
    property int separatorHeight: 13

    width: parent.width
    height: row.isSeparator ? row.separatorHeight : row.rowHeight

    readonly property bool disabled: modelData.enabled === false
    readonly property bool isSeparator: !!modelData.separator
    readonly property bool hasDetail: !!modelData.detail
    readonly property bool hasSubmenu: !!modelData.children

    Rectangle {
        visible: row.isSeparator
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        height: 1
        color: Theme.var.borderSoft
    }

    Item {
        id: iconSlot

        visible: !row.isSeparator
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        width: 16
        height: 16

        Icon {
            anchors.centerIn: parent
            name: row.modelData.icon || ""
            size: 13
            color: Theme.var.textMuted
        }
    }

    Text {
        visible: !row.isSeparator
        anchors.left: iconSlot.right
        anchors.leftMargin: 10
        anchors.right: row.hasDetail ? detailRow.left : (row.hasSubmenu ? submenuArrow.left : parent.right)
        anchors.rightMargin: row.hasDetail || row.hasSubmenu ? 8 : 12
        anchors.verticalCenter: parent.verticalCenter
        text: modelData.text || ""
        color: row.disabled ? Theme.var.textMuted : Theme.var.text
        font.family: Theme.var.fontSans
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    // Marks a row that opens a nested level of the menu.
    Text {
        id: submenuArrow

        visible: row.hasSubmenu
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: "›"
        color: Theme.var.textMuted
        font.family: Theme.var.fontSans
        font.pixelSize: 12
    }

    // The referenced parameter's label and, when relevant, its
    // component index, shown apart from the fixed "Paste" action so
    // the varying part of the row stands out. The index appears as a
    // small tag beside the label.
    Row {
        id: detailRow

        visible: row.hasDetail
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 5

        readonly property color textColor: row.disabled ? Theme.var.textMuted : Theme.var.textLabel

        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, indexBadge.visible ? 65 : 90)
            text: modelData.detail || ""
            color: detailRow.textColor
            font.family: Theme.var.fontSans
            font.italic: true
            font.pixelSize: 11
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
        }

        Rectangle {
            id: indexBadge

            visible: modelData.detailIndex >= 0
            anchors.verticalCenter: parent.verticalCenter
            radius: 3
            color: Theme.var.textFaint
            width: indexLabel.implicitWidth + 6
            height: indexLabel.implicitHeight + 3

            Text {
                id: indexLabel
                anchors.centerIn: parent
                text: modelData.detailIndex
                font.family: Theme.var.fontMono
                font.bold: true
                font.pixelSize: 9
                color: detailRow.textColor
            }
        }
    }
}
