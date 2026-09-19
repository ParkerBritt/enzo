import QtQuick
import QtQuick.Layouts
import Enzo
import "../Style"

// A group of parameters, stacked in a column or laid out in a row.
Column {
    id: group

    required property var item
    readonly property var members: item ? item.children : []
    readonly property bool horizontal: item && item.horizontal

    // Whether the label sits in the label column beside the row instead of above it.
    readonly property bool labelInline: horizontal && item.labelInline && !item.labelHidden

    // Stacked members line up to one label column, shared with the panel.
    property real labelColumnWidth: stacked.implicitLabelWidth
    readonly property real implicitLabelWidth: {
        if (labelInline)
            return Math.ceil(inlineLabelMetrics.advanceWidth);
        return horizontal ? 0 : stacked.implicitLabelWidth;
    }

    // Space between the inline label and the row.
    property real labelGap: 0

    width: parent ? parent.width : 0
    spacing: 6

    Text {
        visible: group.item && !group.item.labelHidden && !group.labelInline
        text: group.item ? group.item.label : ""
        color: Theme.var.textLabel
        font.family: Theme.var.fontSans
        font.pixelSize: 10
        font.weight: Font.Bold
        font.capitalization: Font.AllUppercase
    }

    ParameterList {
        id: stacked
        visible: !group.horizontal
        width: group.width
        model: group.horizontal ? [] : group.members
        labelColumnWidth: group.labelColumnWidth
    }

    TextMetrics {
        id: inlineLabelMetrics
        text: group.labelInline ? group.item.label : ""
        font: inlineLabel.font
    }

    RowLayout {
        visible: group.horizontal
        width: group.width
        spacing: 8

        Text {
            id: inlineLabel
            visible: group.labelInline
            Layout.preferredWidth: group.labelColumnWidth
            Layout.preferredHeight: Constants.parameterHeight
            Layout.rightMargin: group.labelGap - parent.spacing
            verticalAlignment: Text.AlignVCenter
            text: group.item ? group.item.label : ""
            color: Theme.var.text
            font.family: Theme.var.fontSans
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Repeater {
            id: memberRepeater
            model: group.horizontal ? group.members : []
            delegate: ParameterRow {
                required property var modelData
                item: modelData
                Layout.fillWidth: implicitWidth === 0
            }

            // Whether a shown member stretches to take the spare width.
            readonly property bool hasStretchingMember: {
                for (let memberIndex = 0; memberIndex < count; ++memberIndex) {
                    const member = itemAt(memberIndex);
                    if (member && member.visible && member.Layout.fillWidth)
                        return true;
                }
                return false;
            }
        }

        // Keeps the members on the left when none of them stretches.
        Item {
            visible: !memberRepeater.hasStretchingMember
            Layout.fillWidth: true
        }
    }
}
