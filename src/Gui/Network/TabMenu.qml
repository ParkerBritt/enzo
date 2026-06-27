import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Popup {
    width: 300
    height: 400
    focus: true
    background: background 

    Rectangle {
        id: background
        color: Theme.panel
        border.color: Theme.border
        radius: 30
        layer.enabled: true
        layer.effect: MultiEffect {
            anchors.fill: background
            shadowEnabled: true
            shadowBlur: 0.7
            shadowOpacity: 0.3
            shadowHorizontalOffset: 2
            shadowVerticalOffset: 2
        }
    }

}
