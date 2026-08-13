import QtQuick

// Opens the shared context menu on right click and wires its revert/paste
// signals to a ParameterItem, for widgets with no floating editor of their
// own (toggle, dropdown, plain text) where a pasted reference or a revert
// applies directly to the item's single component.
//
// e.g.
//   ParameterContextMenuTrigger {
//       anchors.fill: parent
//       item: root.item
//   }
Item {
    id: trigger

    required property var item

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: mouse => contextMenu.popup(mouse.x, mouse.y)
    }

    ParameterContextMenu {
        id: contextMenu
        nodeName: trigger.item ? trigger.item.nodeName : ""
        paramName: trigger.item ? trigger.item.name : ""
        paramLabel: trigger.item ? trigger.item.label : ""
        kind: trigger.item ? trigger.item.kind : ""
        hasExpression: trigger.item ? trigger.item.hasExpression : false
        value: trigger.item ? trigger.item.value : undefined
        expression: trigger.item ? trigger.item.expression : ""
        editable: false
        onRevertRequested: {
            trigger.item.beginEdit();
            trigger.item.clearExpressionAt(0);
            trigger.item.commitEdit();
        }
        onPasteRequested: expr => {
            trigger.item.beginEdit();
            trigger.item.setExpressionAt(0, expr);
            trigger.item.commitEdit();
        }
        onPasteValueRequested: v => {
            trigger.item.beginEdit();
            trigger.item.setValueAt(0, v);
            trigger.item.commitEdit();
        }
    }
}
