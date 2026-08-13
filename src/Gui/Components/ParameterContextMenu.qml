import QtQuick
import QtQuick.Controls
import Enzo
import "../Style"

// Right-click menu for a parameter, offering expression editing alongside
// copying and pasting another parameter's value, its expression source, or a
// reference to it as prm()/prmI()/prmS(). Edit, Copy, Ref and Revert sit as
// icon buttons in a header above the ordinary text menu.
//
// e.g.
//   ParameterContextMenu {
//       id: menu
//       paramName: "amp"
//       paramLabel: "Density"
//       kind: "float"
//       hasExpression: item.hasExpressionAt(0)
//       value: item.valueAt(0)
//       expression: item.expressionAt(0)
//       onEditRequested: openEditor()
//       onRevertRequested: item.clearExpressionAt(0)
//       onPasteRequested: expr => item.setExpressionAt(0, expr)
//       onPasteValueRequested: v => item.setValueAt(0, v)
//   }
//   menu.popup(mouse.x, mouse.y)
PopupList {
    id: menu

    // This parameter's own identity, and which component when it is one axis
    // of a vector, used both to fill the clipboard and to name a paste target
    // in a reference back to it. The node name qualifies the reference so it
    // still resolves once the panel moves on to another node.
    property string nodeName: ""
    property string paramName: ""
    property string paramLabel: ""
    property int componentIndex: 0
    property string kind: ""

    property bool hasExpression: false

    // This component's current literal value and, when hasExpression is set,
    // the raw expression source behind it. Filled in by the caller and read
    // back out on Copy so the clipboard can offer both a value and an
    // expression paste, not just a reference.
    property var value: undefined
    property string expression: ""

    // Whether this parameter has a floating editor to open, true only for the
    // kinds that already show an expression pill.
    property bool editable: true

    signal editRequested
    signal revertRequested
    signal pasteRequested(string expression)
    signal pasteValueRequested(var value)

    // The function a pasted reference must call, matching this parameter's own
    // value type since that is what its expression has to produce.
    readonly property string referenceFn: {
        if (menu.kind === "float" || menu.kind === "xyz")
            return "prm";
        if (menu.kind === "string" || menu.kind === "dropdown")
            return "prmS";
        return "prmI";
    }

    rowHeight: 30
    implicitWidth: 220
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    readonly property bool pastingOntoSelf: ParameterClipboard.nodeName === menu.nodeName && ParameterClipboard.paramName === menu.paramName && ParameterClipboard.index === menu.componentIndex
    readonly property bool canPaste: ParameterClipboard.hasValue && !menu.pastingOntoSelf

    // Height of the icon button row shown above the ordinary menu entries.
    headerHeight: 58
    header: headerComponent

    // A separator is a thin rule, not a full row, so it takes far less height
    // than the entries around it.
    readonly property int separatorHeight: 13
    rowHeightAt: index => {
        const entry = menu.model[index];
        return entry && entry.separator ? menu.separatorHeight : menu.rowHeight;
    }

    // The paste submenu is wider than the ordinary rows, to leave room for
    // its detail column.
    submenuWidth: 240

    onActivated: index => menu.runLeafAction(index)

    // Opens at a point in the caller's own coordinates, rebuilding the entries
    // fresh so the paste row reflects whatever the clipboard currently holds.
    function popup(px, py) {
        // Cleared so a row highlighted in a previous opening doesn't appear
        // pre-hovered, and so the paste submenu loads fresh instead of
        // staying stuck on its now-stale instance.
        highlightedIndex = -1;
        model = buildEntries();
        x = px;
        y = py;
        open();
    }

    function buildEntries() {
        const entries = [];

        entries.push({
            separator: true
        });

        entries.push({
            text: "Paste",
            icon: "clipboard-paste",
            children: menu.buildPasteEntries()
        });

        return entries;
    }

    function buildPasteEntries() {
        const clip = ParameterClipboard;
        const entries = [];

        // The target casts the pasted value to its own type (numeric
        // <-> numeric converts, anything else falls back to that type's
        // default), so this is offered regardless of the source's kind.
        entries.push({
            text: "Paste Value",
            detail: clip.hasValue ? String(clip.value) : "",
            icon: "clipboard",
            enabled: menu.canPaste,
            action: () => menu.pasteValueRequested(clip.value)
        });

        entries.push({
            text: "Paste Relative Reference",
            detail: clip.hasValue ? clip.paramLabel : "",
            // -1 for a parameter with only one component, where the index adds
            // nothing the label doesn't already say.
            detailIndex: clip.hasValue && clip.index > 0 ? clip.index : -1,
            icon: "link",
            enabled: menu.canPaste,
            action: () => menu.pasteReference()
        });

        entries.push({
            text: "Paste Expression",
            detail: clip.expression,
            icon: "code",
            enabled: menu.canPaste && clip.expression.length > 0,
            action: () => menu.pasteRequested(clip.expression)
        });

        return entries;
    }

    // Builds a prm()/prmI()/prmS() reference to the copied parameter and
    // hands it to the caller.
    function pasteReference() {
        const path = ParameterClipboard.nodeName + "." + ParameterClipboard.paramName;
        const args = ParameterClipboard.index > 0 ? ('"' + path + '", ' + ParameterClipboard.index) : ('"' + path + '"');
        menu.pasteRequested(menu.referenceFn + "(" + args + ")");
    }

    // The icon button row shown above the ordinary menu entries.
    property Component headerComponent: Component {
        Item {
            anchors.fill: parent

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                ParameterContextMenuAction {
                    iconName: "square-pen"
                    label: "Edit"
                    visible: menu.editable
                    onTriggered: {
                        menu.editRequested();
                        menu.close();
                    }
                }

                ParameterContextMenuAction {
                    iconName: "copy"
                    label: "Copy"
                    onTriggered: {
                        ParameterClipboard.copy(menu.nodeName, menu.paramName, menu.paramLabel, menu.componentIndex, menu.kind, menu.value, menu.hasExpression ? menu.expression : "");
                        menu.close();
                    }
                }

                ParameterContextMenuAction {
                    iconName: "clipboard-paste"
                    label: "Ref"
                    enabled: menu.canPaste
                    tooltip: ParameterClipboard.hasValue ? ParameterClipboard.paramLabel : ""
                    onTriggered: {
                        menu.pasteReference();
                        menu.close();
                    }
                }

                ParameterContextMenuAction {
                    iconName: "rotate-ccw"
                    label: "Revert"
                    enabled: menu.hasExpression
                    onTriggered: {
                        menu.revertRequested();
                        menu.close();
                    }
                }
            }
        }
    }

    delegate: Component {
        ParameterContextMenuRow {
            rowHeight: menu.rowHeight
            separatorHeight: menu.separatorHeight
        }
    }
}
