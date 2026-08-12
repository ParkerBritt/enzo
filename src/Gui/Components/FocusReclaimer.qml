import QtQuick

// Gives `target` keyboard focus whenever the mouse rests over `area`, without
// ever taking focus away from a text field being actively edited elsewhere.
Item {
    id: root

    required property Item target
    required property MouseArea area

    property Item previousFocusItem: null

    function isEditingText(item) {
        return item instanceof TextInput || item instanceof TextEdit;
    }

    function reclaim() {
        if (area.containsMouse && !isEditingText(target.Window.activeFocusItem))
            target.forceActiveFocus();
    }

    Connections {
        target: root.area
        function onPositionChanged() { root.reclaim(); }
    }

    // Reclaims only on the transition away from a text field. `target` and its
    // counterparts elsewhere can overlap in screen space, so reclaiming on
    // every focus change would let them bounce focus back and forth forever.
    Connections {
        target: root.target.Window
        function onActiveFocusItemChanged() {
            const wasEditingText = root.isEditingText(root.previousFocusItem);
            root.previousFocusItem = root.target.Window.activeFocusItem;
            if (wasEditingText)
                root.reclaim();
        }
    }
}
