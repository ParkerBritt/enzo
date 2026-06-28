import QtQuick.Controls

// Picks one option, storing its token as the value.
ComboBox {
    required property var item

    implicitHeight: 24
    model: item.options
    currentIndex: item.optionTokens.indexOf(item.value)
    onActivated: (i) => item.value = item.optionTokens[i]
}
