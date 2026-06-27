import QtQuick
import QtQuick.Controls

Popup {
    width: 500
    height: 500
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    background: background 

    Rectangle {
        id: background
        color: Theme.panel
        border.color: Theme.border
        radius: 30
    }
}
