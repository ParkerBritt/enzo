import QtQuick
import QtQuick.Controls
import Enzo
import "../Components"
import "../Style"

// A multi line code editor for a string parameter.
Column {
    id: parameter

    required property var item

    // Spaces a tab press inserts.
    readonly property string indent: "    "

    readonly property int padding: 8
    readonly property int lineHeight: Math.ceil(metrics.height)

    readonly property int lineCount: Math.max(field.lineCount, 12)

    width: parent ? parent.width : 0
    spacing: 6

    // Stores text as the parameter's value, as one undo step.
    function commitValue(text) {
        if (!parameter.item || parameter.item.value === text)
            return;
        parameter.item.beginEdit();
        parameter.item.value = text;
        parameter.item.commitEdit();
    }

    FontMetrics {
        id: metrics
        font.family: Theme.var.fontMono
        font.pixelSize: 14
    }

    TextMetrics {
        id: gutterMetrics
        font: metrics.font
        text: String(parameter.lineCount)
    }

    ParameterLabel {
        item: parameter.item
    }

    Rectangle {
        width: parameter.width
        height: parameter.lineCount * parameter.lineHeight + 2 * parameter.padding
        radius: Theme.parameter.borderRadius
        color: Theme.parameter.backgroundColor
        border.color: Theme.parameter.lineColor

        Column {
            id: gutter

            x: parameter.padding
            y: parameter.padding
            width: Math.ceil(gutterMetrics.advanceWidth)

            Repeater {
                model: parameter.lineCount

                Text {
                    width: gutter.width
                    height: parameter.lineHeight
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    text: index + 1
                    color: Theme.codeEditor.gutterTextColor
                    font: metrics.font
                }
            }
        }

        ScrollView {
            anchors.fill: parent
            anchors.margins: parameter.padding
            anchors.leftMargin: parameter.padding + gutter.width + 10
            clip: true

            TextArea {
                id: field

                padding: 0
                background: null
                text: parameter.item ? parameter.item.value : ""
                color: Theme.var.text
                font: metrics.font
                wrapMode: TextArea.NoWrap
                selectByMouse: true
                persistentSelection: true

                onActiveFocusChanged: {
                    if (!activeFocus)
                        parameter.commitValue(text);
                }

                Keys.onPressed: event => {
                    const isReturn = event.key === Qt.Key_Return || event.key === Qt.Key_Enter;
                    if (isReturn && (event.modifiers & Qt.ControlModifier)) {
                        parameter.commitValue(text);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Tab) {
                        insert(cursorPosition, parameter.indent);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Escape) {
                        text = parameter.item ? parameter.item.value : "";
                        focus = false;
                        event.accepted = true;
                    }
                }
            }
        }

        ParameterContextMenuTrigger {
            anchors.fill: parent
            item: parameter.item
        }
    }
}
