import QtQuick
import QtQuick.Controls
import Enzo
import "../Components"

// One level of menu entries floating beneath its title. Built on the shared
// PopupList so its reveal, highlight and keyboard travel match the other popups.
//
// An entry is a plain object { text, action, enabled, separator }. A leaf carries
// an action run on choice. Icons and submenu children land in a later pass.
PopupList {
    id: list

    property var entries: []

    model: entries
    rowHeight: 26
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    canHighlight: (index) => {
        const entry = entries[index]
        return entry !== undefined && entry.enabled !== false && !entry.separator
    }

    onActivated: (index) => {
        const entry = entries[index]
        if (entry && entry.action) entry.action()
        list.close()
    }

    delegate: Component {
        Item {
            id: row

            required property int index
            required property var modelData

            width: list.availableWidth
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
        }
    }
}
