import QtQuick
import Enzo
import "../Components"
import "../Style"

// A row of buttons that picks one option and stores its token as the value.
Item {
    id: root

    required property var item

    readonly property var labels: item ? item.options : []
    readonly property var icons: item ? item.optionIcons : []
    readonly property var tokens: item ? item.optionTokens : []
    readonly property int optionCount: tokens.length
    readonly property int currentIndex: tokens.indexOf(item ? item.value : "")

    readonly property int buttonPadding: 10
    readonly property int iconSize: 14
    readonly property int iconGap: 5
    readonly property int iconButtonWidth: 30

    readonly property bool everyOptionHasIcon: icons.length > 0 && icons.every(name => name !== "")

    // Returns the width an option's icon takes beside its label, 0 when it has none.
    function getIconSpace(index) {
        return icons[index] ? iconSize + iconGap : 0;
    }

    // Returns the width a button needs to show its icon and label in full.
    function getButtonWidth(index) {
        return metrics.advanceWidth(labels[index]) + getIconSpace(index) + buttonPadding * 2;
    }

    readonly property real widestButtonWidth: {
        let widest = 0;
        for (let index = 0; index < optionCount; ++index)
            widest = Math.max(widest, getButtonWidth(index));
        return Math.ceil(widest);
    }

    readonly property bool labelsFit: widestButtonWidth * optionCount <= width
    readonly property bool iconsFit: everyOptionHasIcon && iconButtonWidth * optionCount <= width
    readonly property bool showsIconsOnly: !labelsFit && iconsFit

    // Wraps the buttons onto more rows when neither the labels nor the icons fit on one.
    readonly property int columnCount: {
        if (labelsFit || showsIconsOnly)
            return Math.max(optionCount, 1);
        return Math.max(1, Math.min(optionCount, Math.floor(width / widestButtonWidth)));
    }
    readonly property int rowCount: Math.max(1, Math.ceil(optionCount / columnCount))
    readonly property real buttonWidth: width / columnCount

    implicitHeight: rowCount * Constants.parameterHeight

    function getButtonX(index) {
        return (index % columnCount) * buttonWidth;
    }

    function getButtonY(index) {
        return Math.floor(index / columnCount) * Constants.parameterHeight;
    }

    // Stores the option at an index as the value, as one undo step.
    function commitIndex(index) {
        if (!item || index === currentIndex) return;
        item.beginEdit();
        item.value = tokens[index];
        item.commitEdit();
    }

    FontMetrics {
        id: metrics

        font.family: Theme.var.fontSans
        font.pixelSize: 12
        font.weight: Font.Medium
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.var.controlRadius
        color: Theme.parameter.backgroundColor
        border.color: Theme.parameter.lineColor
    }

    // The fill behind the chosen button.
    Rectangle {
        readonly property int inset: 2

        visible: root.currentIndex >= 0
        x: root.getButtonX(Math.max(root.currentIndex, 0)) + inset
        y: root.getButtonY(Math.max(root.currentIndex, 0)) + inset
        width: root.buttonWidth - inset * 2
        height: Constants.parameterHeight - inset * 2
        radius: Math.max(Theme.var.controlRadius - inset, 0)
        color: Theme.buttonStrip.selectedColor

        Behavior on x { NumberAnimation { duration: 110 * interfaceSettings.durationScale; easing.type: Easing.OutCubic } }
        Behavior on y { NumberAnimation { duration: 110 * interfaceSettings.durationScale; easing.type: Easing.OutCubic } }
    }

    Repeater {
        model: root.optionCount

        Item {
            id: button

            required property int index

            readonly property bool selected: index === root.currentIndex
            readonly property string icon: root.icons[index] || ""
            readonly property color contentColor: selected || mouse.containsMouse ? Theme.var.text : Theme.var.textLabel

            x: root.getButtonX(index)
            y: root.getButtonY(index)
            width: root.buttonWidth
            height: Constants.parameterHeight

            Row {
                anchors.centerIn: parent
                spacing: root.iconGap

                Icon {
                    visible: button.icon !== ""
                    name: button.icon
                    size: root.iconSize
                    color: button.contentColor
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    visible: !root.showsIconsOnly
                    width: Math.min(implicitWidth, button.width - root.buttonPadding * 2 - root.getIconSpace(button.index))
                    text: root.labels[button.index]
                    color: button.contentColor
                    font: metrics.font
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: mouse

                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.commitIndex(button.index)
            }

            Tooltip {
                text: root.labels[button.index]
                visible: root.showsIconsOnly && mouse.containsMouse
            }
        }
    }

    ParameterContextMenuTrigger {
        anchors.fill: parent
        item: root.item
    }
}
