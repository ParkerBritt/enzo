import QtQuick
import "../Components"
import "../Style"

// Picks one option, storing its token as the value.
Dropdown {
    id: root

    required property var item

    readonly property var tokens: item ? item.optionTokens : []

    implicitHeight: Constants.parameterHeight

    labels: item ? item.options : []
    currentIndex: tokens.indexOf(item ? item.value : "")
    onActivated: (index) => {
        item.beginEdit();
        item.value = tokens[index];
        item.commitEdit();
    }

    ParameterContextMenuTrigger {
        anchors.fill: parent
        item: root.item
    }
}
