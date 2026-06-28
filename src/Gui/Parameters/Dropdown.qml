import QtQuick.Controls

// Picks one option, storing its token as the value.
ComboBox {
    required property var item

    implicitHeight: 24
    model: item ? item.options : []
    currentIndex: item ? item.optionTokens.indexOf(item.value) : -1
    onActivated: (i) => item.value = item.optionTokens[i]
}
