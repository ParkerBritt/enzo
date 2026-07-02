import QtQuick
import QtQuick.Controls
import Enzo
import "../Components"

// One level of menu entries floating beneath its title. Built on the shared
// PopupList so its reveal, highlight and keyboard travel match the other popups.
//
// An entry is a plain object { text, action, enabled, separator, children }. A
// leaf carries an action run on choice, while children open a nested level of
// the same popup beside the row.
PopupList {
    id: list

    property var entries: []

    // Row whose children are open as a nested level, or -1 when none.
    readonly property int submenuIndex:
        entries[highlightedIndex]?.children ? highlightedIndex : -1

    // Fires when a leaf is chosen at any depth so every level closes.
    signal leafChosen()
    onLeafChosen: list.close()

    model: entries
    rowHeight: 26
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    canHighlight: (index) => {
        const entry = entries[index]
        return entry !== undefined && entry.enabled !== false && !entry.separator
    }

    onActivated: (index) => {
        const entry = entries[index]
        if (entry.children) return
        if (entry.action) entry.action()
        list.leafChosen()
    }

    // A component cannot instantiate itself directly, so the nested level loads on demand.
    Loader {
        active: list.submenuIndex >= 0
        source: "MenuPopup.qml"
        onLoaded: {
            item.parent = list.contentItem
            item.x = Qt.binding(() => list.contentItem.width + list.padding)
            item.y = Qt.binding(() => list.submenuIndex * list.rowHeight - list.padding)
            item.entries = Qt.binding(() => list.entries[list.submenuIndex]?.children ?? [])
            item.leafChosen.connect(list.leafChosen)
            item.open()
        }
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
