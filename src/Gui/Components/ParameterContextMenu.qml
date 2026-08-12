import QtQuick
import QtQuick.Controls
import Enzo
import "../Style"

// Right-click menu for a parameter, offering expression editing alongside
// copying and pasting a reference to another parameter as prm()/prmI()/prmS().
//
// e.g.
//   ParameterContextMenu {
//       id: menu
//       paramName: "amp"
//       paramLabel: "Density"
//       kind: "float"
//       hasExpression: item.hasExpressionAt(0)
//       onEditRequested: openEditor()
//       onRevertRequested: item.clearExpressionAt(0)
//       onPasteRequested: expr => item.setExpressionAt(0, expr)
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

    // Whether this parameter has a floating editor to open, true only for the
    // kinds that already show an expression pill.
    property bool editable: true

    signal editRequested
    signal revertRequested
    signal pasteRequested(string expression)

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

    // A separator is a thin rule, not a full row, so it takes far less height
    // than the entries around it.
    readonly property int separatorHeight: 13
    rowHeightAt: (index) => {
        const entry = menu.model[index];
        return entry && entry.separator ? menu.separatorHeight : menu.rowHeight;
    }

    canHighlight: (index) => {
        const entry = menu.model[index];
        return entry !== undefined && entry.enabled !== false && !entry.separator;
    }

    onActivated: index => {
        const entry = menu.model[index];
        if (entry.action)
            entry.action();
        menu.close();
    }

    // Opens at a point in the caller's own coordinates, rebuilding the entries
    // fresh so the paste row reflects whatever the clipboard currently holds.
    function popup(px, py) {
        model = buildEntries();
        x = px;
        y = py;
        open();
    }

    function buildEntries() {
        const clip = ParameterClipboard;
        const entries = [];

        if (menu.editable)
            entries.push({
                text: "Edit Expression…",
                icon: "fx",
                action: () => menu.editRequested(),
            });
        if (menu.hasExpression)
            entries.push({
                text: "Revert to Value",
                icon: "rotate-ccw",
                action: () => menu.revertRequested(),
            });

        entries.push({ separator: true });

        entries.push({
            text: "Copy Parameter",
            icon: "copy",
            action: () => clip.copy(menu.nodeName, menu.paramName, menu.paramLabel, menu.componentIndex),
        });

        const pastingOntoSelf = clip.nodeName === menu.nodeName && clip.paramName === menu.paramName && clip.index === menu.componentIndex;
        entries.push({
            text: "Paste",
            detail: clip.hasValue ? clip.paramLabel : "",
            // -1 for a parameter with only one component, where the index adds
            // nothing the label doesn't already say.
            detailIndex: clip.hasValue && clip.index > 0 ? clip.index : -1,
            icon: "clipboard-paste",
            enabled: clip.hasValue && !pastingOntoSelf,
            action: () => {
                const path = clip.nodeName + "." + clip.paramName;
                const args = clip.index > 0 ? ('"' + path + '", ' + clip.index) : ('"' + path + '"');
                menu.pasteRequested(menu.referenceFn + "(" + args + ")");
            },
        });

        return entries;
    }

    delegate: Component {
        Item {
            id: row

            required property int index
            required property var modelData

            width: menu.availableWidth
            height: menu.rowHeightAt(index)

            readonly property bool disabled: modelData.enabled === false
            readonly property bool isSeparator: !!modelData.separator
            readonly property bool hasDetail: !!modelData.detail

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

            // Names the row's action. The expression editor entry uses the same
            // "fx" mark as the parameter's own expression badge instead of an
            // icon from the shared set.
            Item {
                id: iconSlot

                visible: !row.isSeparator
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16

                Text {
                    visible: row.modelData.icon === "fx"
                    anchors.centerIn: parent
                    text: "fx"
                    font.italic: true
                    font.bold: true
                    font.pixelSize: 10
                    font.family: Theme.var.fontMono
                    color: row.disabled ? Theme.var.textMuted : Theme.var.accentBright
                }

                Icon {
                    readonly property bool isIconName: row.modelData.icon !== undefined && row.modelData.icon !== "fx"

                    visible: isIconName
                    anchors.centerIn: parent
                    name: isIconName ? row.modelData.icon : ""
                    size: 13
                    color: Theme.var.textMuted
                }
            }

            Text {
                visible: !row.isSeparator
                anchors.left: iconSlot.right
                anchors.leftMargin: 10
                anchors.right: row.hasDetail ? detailRow.left : parent.right
                anchors.rightMargin: row.hasDetail ? 8 : 12
                anchors.verticalCenter: parent.verticalCenter
                text: modelData.text || ""
                color: row.disabled ? Theme.var.textMuted : Theme.var.text
                font.family: Theme.var.fontSans
                font.pixelSize: 12
                elide: Text.ElideRight
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
    }
}
