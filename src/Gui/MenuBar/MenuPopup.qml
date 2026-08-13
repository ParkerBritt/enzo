import QtQuick
import QtQuick.Controls
import Enzo
import "../Components"

// One level of menu entries floating beneath its title, including any nested
// levels opened from an entry with children. Built on the shared PopupList so
// its reveal, highlight, keyboard travel and submenu nesting match the other
// popups.
//
// An entry is a plain object { text, action, enabled, separator, children }.
PopupList {
    id: list

    property var entries: []

    model: entries
    rowHeight: 26
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    onActivated: (index) => list.runLeafAction(index)

    delegate: Component {
        Item {
            id: row

            required property int index
            required property var modelData

            width: parent.width
            height: list.rowHeight

            readonly property bool disabled: modelData.enabled === false

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: modelData.text || ""
                color: row.disabled ? Theme.var.textMuted : Theme.var.text
                font.family: Theme.var.fontSans
                font.pixelSize: 12
                elide: Text.ElideRight
            }

            // Arrow marking a row that opens a nested level.
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                visible: row.modelData.children !== undefined
                text: "›"
                color: Theme.var.textMuted
                font.family: Theme.var.fontSans
                font.pixelSize: 12
            }
        }
    }
}
