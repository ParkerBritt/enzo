import QtQuick

// Horizontal bar of menu titles backed by the shared menu popup. Pressing a title
// opens its menu beneath it and pressing it again closes. While a menu is open,
// moving onto another title switches to it like a native menu bar.
//
// Each menu is { title, entries } where entries is a function returning the row
// list, rebuilt every time the menu opens.
Item {
    id: bar

    implicitHeight: 32

    property var menus: []

    // Title whose menu is open, or -1 when the bar is closed.
    property int openIndex: -1

    function openMenu(index) {
        if (index === openIndex) return
        // A title with nothing to show leaves the bar closed.
        const entries = bar.menus[index].entries
        if (entries && entries().length === 0) return
        openIndex = index
    }

    function closeMenu() { openIndex = -1 }

    Rectangle {
        anchors.fill: parent
        color: Theme.menuBar
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Repeater {
            model: bar.menus

            delegate: Rectangle {
                id: title

                required property int index
                required property var modelData

                readonly property bool open: bar.openIndex === index

                width: label.implicitWidth + 18
                height: 22
                radius: 6
                color: open || hover.hovered ? Theme.bsoft : "transparent"

                Text {
                    id: label
                    anchors.centerIn: parent
                    text: title.modelData.title
                    color: Theme.text
                    font.family: Theme.fontUi
                    font.pixelSize: 12
                }

                HoverHandler {
                    id: hover
                    onHoveredChanged: if (hovered && bar.openIndex >= 0 && !title.open) bar.openMenu(index)
                }
                TapHandler {
                    onTapped: title.open ? bar.closeMenu() : bar.openMenu(index)
                }

                onOpenChanged: open ? popup.open() : popup.close()

                MenuPopup {
                    id: popup
                    y: title.height + 4
                    entries: title.modelData.entries ? title.modelData.entries() : []
                    onClosed: if (bar.openIndex === title.index) bar.closeMenu()
                }
            }
        }
    }
}
