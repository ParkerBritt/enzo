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

    // The parameter's own live value. The preview below calls evaluator as a
    // plain function, which QML can't see into, so this is read purely to
    // give the preview binding something to react to when an upstream
    // dependency changes it out from under an open editor.
    property real liveValue: 0

    signal committed(string expression)
    signal reverted

    padding: 0
    focus: true
    implicitWidth: 300
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // How much of the popup's height is currently revealed, from 0 to 1.
    property real unrollProgress: 1
    opacity: unrollProgress

    enter: Transition {
        NumberAnimation {
            target: root
            property: "unrollProgress"
            from: 0
            to: 1
            duration: 300
            easing.type: Easing.OutCubic
        }
    }
    exit: Transition {
        NumberAnimation {
            target: root
            property: "unrollProgress"
            to: 0
            duration: 250
            easing.type: Easing.InCubic
        }
    }

    // Functions expressions can actually call, offered as one click inserts.
    readonly property var insertTokens: ["prm()", "prmI()", "prmS()"]

    // Live result of whatever text currently sits in the code field.
    readonly property var preview: {
        root.liveValue;
        return root.evaluator ? root.evaluator(codeField.text) : null;
    }
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
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.7
            shadowOpacity: 0.35
            shadowVerticalOffset: 4
        }
    }

    // Height tracks unrollProgress while clip hides the rest of the column,
    // so the panel reveals top-down at full size.
    contentItem: Item {
        implicitWidth: root.availableWidth
        implicitHeight: column.height * root.unrollProgress
        clip: true

        Column {
            id: column
            width: parent.width

            // Title strip, styled to match the floating parameter panel's header.
            Rectangle {
                id: header
                width: parent.width
                height: 44
                topLeftRadius: 12
                topRightRadius: 12
                color: Theme.var.surfaceRaised

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

            Column {
                width: parent.width
                spacing: 10
                topPadding: 13
                bottomPadding: 13
                leftPadding: 13
                rightPadding: 13

                Rectangle {
                    id: codeBox
                    width: parent.width - parent.leftPadding - parent.rightPadding
                    height: codeColumn.height
                    radius: 9
                    color: Theme.codeEditor.backgroundColor
                    border.color: Theme.var.borderSoft
                    clip: true

                    Column {
                        id: codeColumn
                        width: parent.width

                        Row {
                            id: codeRow
                            width: parent.width
                            height: Math.max(108, codeField.contentHeight + 22)

                            Item {
                                id: gutter
                                width: 24
                                height: parent.height

                                Rectangle {
                                    anchors.right: parent.right
                                    width: 1
                                    height: parent.height
                                    color: Theme.codeEditor.gutterBorderColor
                                }

                                Column {
                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    anchors.topMargin: 11
                                    anchors.rightMargin: 11

                                    Repeater {
                                        model: Math.max(3, codeField.lineCount)

                                        delegate: Text {
                                            required property int index
                                            width: 27
                                            horizontalAlignment: Text.AlignRight
                                            text: (index + 1).toString()
                                            color: Theme.codeEditor.gutterTextColor
                                            font.family: Theme.var.fontMono
                                            font.pixelSize: 12
                                        }
                                    }
                                }
                            }

                            TextEdit {
                                id: codeField
                                width: parent.width - gutter.width
                                height: parent.height
                                topPadding: 11
                                bottomPadding: 11
                                leftPadding: 13
                                rightPadding: 13
                                wrapMode: TextEdit.WrapAnywhere
                                selectByMouse: true
                                color: Theme.var.text
                                font.family: Theme.var.fontMono
                                font.pixelSize: 12
                                Keys.onEscapePressed: root.close()
                                Keys.onPressed: event => {
                                    const isReturn = event.key === Qt.Key_Return || event.key === Qt.Key_Enter;
                                    if (isReturn && (event.modifiers & Qt.ControlModifier)) {
                                        root.committed(text);
                                        root.close();
                                        event.accepted = true;
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Theme.var.borderSoft
                        }

                        Row {
                            topPadding: 7
                            bottomPadding: 7
                            leftPadding: 13
                            rightPadding: 13
                            spacing: 9

                            Text {
                                text: root.previewInvalid ? "⚠" : "="
                                color: root.previewInvalid ? Theme.expression.invalidResultColor : Theme.expression.resultColor
                                font.pixelSize: 12
                            }
                            Text {
                                visible: root.previewInvalid
                                text: "defaulting to"
                                color: Theme.var.textMuted
                                font.family: Theme.var.fontSans
                                font.pixelSize: 10
                            }
                            Text {
                                text: root.previewText
                                color: root.previewInvalid ? Theme.expression.invalidResultColor : Theme.expression.resultColor
                                font.family: Theme.var.fontMono
                                font.pixelSize: 12
                            }
                        }
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

            // Action strip for committing or discarding the expression.
            Rectangle {
                id: footer
                width: parent.width
                height: 46
                bottomLeftRadius: 12
                bottomRightRadius: 12
                color: Theme.var.surfaceHeader

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

        // Drawn last so the border isn't painted over by the header and footer fills.
        Rectangle {
            anchors.fill: parent
            radius: 12
            color: "transparent"
            border.color: Theme.var.borderSoft
        }
    }
}
