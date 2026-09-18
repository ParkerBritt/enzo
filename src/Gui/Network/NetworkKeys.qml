import QtQuick

// The key bindings of the network.
Item {
    id: root

    // The tools the held keys switch.
    property var nodeDrop
    property var cutter

    // The controller an escape cancels the in-progress link on.
    property var linkDrag

    // Asks for the node menu.
    signal menuRequested

    Keys.onTabPressed: root.menuRequested()

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Shift)
            nodeDrop.bypassHeld = true;
        else if (event.key === Qt.Key_C)
            cutter.held = true;
        else if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace)
            network.deleteSelected();
        else if (event.key === Qt.Key_Escape && linkDrag.linking)
            linkDrag.cancel();
        else if (event.key === Qt.Key_R)
            network.setDisplayNodeToPrimary();
    }

    Keys.onReleased: event => {
        if (event.isAutoRepeat)
            return;
        if (event.key === Qt.Key_Shift)
            nodeDrop.bypassHeld = false;
        else if (event.key === Qt.Key_C)
            cutter.held = false;
    }

    // Clears the held keys.
    function clearHeld() {
        nodeDrop.bypassHeld = false;
        cutter.held = false;
    }
}
