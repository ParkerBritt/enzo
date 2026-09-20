import QtQuick
import Enzo

// The label a control draws above itself, for kinds the row gives no side label.
Text {
    required property var item

    visible: item && !item.labelHidden
    text: item ? item.label : ""
    color: Theme.var.text
    font.family: Theme.var.fontSans
    font.pixelSize: 12
}
