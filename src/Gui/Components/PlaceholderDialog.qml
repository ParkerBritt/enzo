import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// Reusable stand-in for a feature that is not built yet. A caller sets the heading
// and opens it to show a centered card naming the feature.
//
// e.g.
//   placeholder.show("Tutorials")
Popup {
    id: root

    property string heading: ""

    function show(text) {
        heading = text
        open()
    }

    anchors.centerIn: Overlay.overlay
    modal: true
    padding: 0
    width: 320

    Overlay.modal: Rectangle { color: "#80000000" }

    background: Rectangle {
        radius: Theme.panelRadius
        color: Theme.panel
        border.color: Theme.border
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.8
            shadowOpacity: 0.4
            shadowVerticalOffset: 4
        }
    }

    contentItem: Column {
        spacing: 14
        topPadding: 26
        bottomPadding: 22
        leftPadding: 24
        rightPadding: 24

        Icon {
            name: "construction"
            size: 26
            color: Theme.accent2
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            width: parent.width - parent.leftPadding - parent.rightPadding
            text: root.heading
            color: Theme.name
            font.family: Theme.fontUi
            font.pixelSize: 15
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width - parent.leftPadding - parent.rightPadding
            text: "Coming soon."
            color: Theme.muted
            font.family: Theme.fontUi
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 96
            height: 28
            radius: 7
            color: closeArea.containsMouse ? Theme.accent : Theme.bsoft
            border.color: Theme.border

            Text {
                anchors.centerIn: parent
                text: "Got it"
                color: Theme.text
                font.family: Theme.fontUi
                font.pixelSize: 12
            }

            MouseArea {
                id: closeArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.close()
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 130; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.94; to: 1; duration: 130; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: 100 }
    }
}
