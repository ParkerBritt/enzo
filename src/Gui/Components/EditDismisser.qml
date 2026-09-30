import QtQuick

// Ends a text edit when a press lands outside the field.
MouseArea {
    id: root

    function isEditingText(item) {
        return item instanceof TextInput || item instanceof TextEdit;
    }

    onPressed: mouse => {
        mouse.accepted = false;
        const field = root.Window.activeFocusItem;
        if (!root.isEditingText(field))
            return;
        const pressInField = field.contains(field.mapFromItem(root, mouse.x, mouse.y));
        if (!pressInField)
            field.focus = false;
    }
}
