import QtQuick
import Enzo
import "../Components"
import "../Style"

Rectangle {
    id: parameter

    required property var item

    implicitHeight: Constants.parameterHeight
    radius: Theme.parameter.borderRadius
    color: Theme.parameter.backgroundColor
    border.color: attributeList.visible ? Theme.var.accentLine : Theme.parameter.lineColor

    // Stores text as the parameter's value, as one undo step.
    function commitValue(text) {
        if (!parameter.item) return;
        parameter.item.beginEdit();
        parameter.item.value = text;
        parameter.item.commitEdit();
        field.text = text;
    }

    TextInput {
        id: field

        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 24
        verticalAlignment: TextInput.AlignVCenter
        clip: true
        text: parameter.item ? parameter.item.value : ""
        color: Theme.var.text
        font.family: Theme.var.fontSans
        font.pixelSize: 12
        onEditingFinished: parameter.commitValue(text)
        onAccepted: focus = false
        Keys.onEscapePressed: {
            text = parameter.item ? parameter.item.value : "";
            focus = false;
        }
    }

    // Lists the attributes on the node's input.
    MouseArea {
        id: picker

        width: 24
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        hoverEnabled: true

        // Opens the list. A press while it is open closes it before this runs.
        onPressed: {
            if (attributeList.visible) return;
            attributeList.model = parameter.item.attributeNames();
            if (attributeList.model.length > 0) attributeList.open();
        }

        Icon {
            name: "chevron-down"
            size: 12
            color: picker.containsMouse || attributeList.visible ? Theme.var.text : Theme.var.textMuted
            anchors.centerIn: parent
            rotation: attributeList.visible ? 180 : 0
            Behavior on rotation {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    PopupList {
        id: attributeList

        x: parameter.width - width
        y: parameter.height + 4
        implicitWidth: Math.max(parameter.width, 150)
        rowHeight: 26
        model: []

        onActivated: index => {
            parameter.commitValue(attributeList.model[index]);
            attributeList.close();
        }

        delegate: Text {
            required property int index
            required property var modelData

            width: attributeList.availableWidth
            height: attributeList.rowHeight
            leftPadding: 10
            rightPadding: 10
            text: modelData
            color: modelData === parameter.item.value ? Theme.var.text : Theme.var.textLabel
            font.family: Theme.var.fontSans
            font.pixelSize: 12
            font.weight: Font.Medium
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    ParameterContextMenuTrigger {
        anchors.fill: parent
        item: parameter.item
    }
}
