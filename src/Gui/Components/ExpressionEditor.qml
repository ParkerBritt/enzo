import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Enzo
import "../Style"

// Floating editor for a parameter's expression, opened from its pill.
//
// e.g.
//   ExpressionEditor {
//       id: editor
//       onCommitted: (text) => item.setExpression(text)
//   }
//   editor.openFor("prm(\"amp\") * 2.3")
Popup {
    id: root

    property string paramLabel: ""
    property string paramKind: ""

    // Evaluates typed text without committing it, returning {value, invalid}.
    property var evaluator: null

    signal committed(string expression)
    signal reverted

    padding: 0
    focus: true
    implicitWidth: 300
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Functions expressions can actually call, offered as one click inserts.
    readonly property var insertTokens: ["prm()", "prmI()", "prmS()"]

    // Live result of whatever text currently sits in the code field.
    readonly property var preview: root.evaluator ? root.evaluator(codeField.text) : null
    readonly property bool previewInvalid: root.preview ? !!root.preview.invalid : false
    readonly property string previewText: {
        if (!root.preview)
            return "";
        const value = root.preview.value;
        if (root.paramKind === "int")
            return Math.round(value).toString();
        if (root.paramKind === "float")
            return Number(value).toFixed(3);
        return String(value);
    }

    function openFor(codeText) {
        open();
        codeField.text = codeText;
        codeField.selectAll();
        codeField.forceActiveFocus();
    }

    background: Rectangle {
        radius: 12
        color: Theme.var.surface
        border.color: Theme.var.border
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.7
            shadowOpacity: 0.35
            shadowVerticalOffset: 4
        }
    }

    contentItem: Column {
        width: root.availableWidth

        Item {
            width: parent.width
            height: 44

            Rectangle {
                id: fxBadge
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                radius: 4
                color: Theme.expression.badgeBackgroundColor
                width: fxLabel.implicitWidth + 10
                height: fxLabel.implicitHeight + 4

                Text {
                    id: fxLabel
                    anchors.centerIn: parent
                    text: "fx"
                    font.italic: true
                    font.bold: true
                    font.family: Theme.var.fontMono
                    font.pixelSize: 9
                    color: Theme.expression.badgeColor
                }
            }

            Column {
                anchors.left: fxBadge.right
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1

                Text {
                    text: root.paramLabel
                    color: Theme.var.textStrong
                    font.family: Theme.var.fontSans
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
                Text {
                    text: root.paramKind
                    color: Theme.var.textMuted
                    font.family: Theme.var.fontMono
                    font.pixelSize: 9
                }
            }

            IconButton {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 26
                height: 26
                name: "x"
                iconSize: 11
                onClicked: root.close()
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.var.border
        }

        Column {
            width: parent.width
            spacing: 10
            topPadding: 13
            bottomPadding: 13
            leftPadding: 13
            rightPadding: 13

            Rectangle {
                width: parent.width - parent.leftPadding - parent.rightPadding
                height: 36
                radius: 9
                color: Theme.var.background
                border.color: Theme.var.borderSoft

                TextInput {
                    id: codeField
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                    selectByMouse: true
                    color: Theme.var.text
                    font.family: Theme.var.fontMono
                    font.pixelSize: 12
                    onAccepted: {
                        root.committed(text);
                        root.close();
                    }
                    Keys.onEscapePressed: root.close()
                }
            }

            Row {
                spacing: 8

                Text {
                    text: root.previewInvalid ? "⚠" : "="
                    color: root.previewInvalid ? Theme.expression.invalidResultColor : Theme.expression.resultColor
                    font.pixelSize: 12
                }
                Text {
                    text: root.previewText
                    color: root.previewInvalid ? Theme.expression.invalidResultColor : Theme.expression.resultColor
                    font.family: Theme.var.fontMono
                    font.pixelSize: 12
                }
            }

            Row {
                spacing: 6

                Repeater {
                    model: root.insertTokens

                    delegate: Rectangle {
                        id: chip
                        required property string modelData

                        radius: 6
                        color: Theme.var.fieldSurface
                        border.color: Theme.var.fieldBorder
                        width: chipText.implicitWidth + 14
                        height: 22

                        Text {
                            id: chipText
                            anchors.centerIn: parent
                            text: chip.modelData
                            color: Theme.var.textLabel
                            font.family: Theme.var.fontMono
                            font.pixelSize: 10
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                codeField.insert(codeField.cursorPosition, chip.modelData);
                                codeField.forceActiveFocus();
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.var.border
        }

        Item {
            width: parent.width
            height: 46

            Rectangle {
                id: revertButton
                anchors.right: doneButton.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: revertText.implicitWidth + 20
                height: 26
                radius: 7
                color: revertMouse.containsMouse ? Theme.var.borderSoft : "transparent"
                border.color: Theme.var.fieldBorder

                Text {
                    id: revertText
                    anchors.centerIn: parent
                    text: "Revert to value"
                    color: Theme.var.textLabel
                    font.family: Theme.var.fontSans
                    font.pixelSize: 11
                }

                MouseArea {
                    id: revertMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.reverted();
                        root.close();
                    }
                }
            }

            Rectangle {
                id: doneButton
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: doneText.implicitWidth + 24
                height: 26
                radius: 7
                color: Theme.expression.badgeBackgroundColor
                border.color: Theme.expression.borderColor

                Text {
                    id: doneText
                    anchors.centerIn: parent
                    text: "Done"
                    color: Theme.expression.badgeColor
                    font.family: Theme.var.fontSans
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.committed(codeField.text);
                        root.close();
                    }
                }
            }
        }
    }
}
