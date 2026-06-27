import QtQuick
import QtQuick.Effects

Item {
    id: root

    width: 80
    height: 20
    property real radius: 5
    property real viewZoom: 1

    Rectangle {
        id: dropShadowRect
        anchors.fill: parent
        radius: root.radius
        color: "#262630"

    }
    MultiEffect
    {
        source: dropShadowRect
        anchors.fill: dropShadowRect
        shadowEnabled: true
        shadowBlur: 0.8
        shadowOpacity: 0.3
        shadowHorizontalOffset: 2
        shadowVerticalOffset: 2
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.panelHeader
        radius: root.radius
        x: -40
        y: 0
        antialiasing: true
        border.pixelAligned: false
        border.color: "#262630"
        border.width: 0.8/root.viewZoom
    }
}
