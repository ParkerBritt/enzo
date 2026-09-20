import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Enzo
import "../Components"

// The question asked when another theme is chosen while the current one holds
// edits that are not written yet.
Popup {
    id: root

    // The theme waiting to be switched to.
    property string targetTheme: ""

    // Apply was chosen, keeping the edits before the switch.
    signal applyChosen

    // Discard was chosen, dropping the edits before the switch.
    signal discardChosen

    function ask(name) {
        root.targetTheme = name;
        root.open();
    }

    anchors.centerIn: Overlay.overlay
    modal: true
    padding: 20
    width: 400

    Overlay.modal: Rectangle {
        color: "#80000000"
    }

    background: Rectangle {
        radius: Theme.var.panelRadius
        color: Theme.var.surfacePanel
        border.color: Theme.var.border
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.8
            shadowOpacity: 0.5
            shadowVerticalOffset: 6
        }
    }

    contentItem: Column {
        spacing: 16

        Column {
            width: parent.width
            spacing: 4

            Text {
                text: "Switching theme"
                color: Theme.var.textMuted
                font.family: Theme.var.fontSans
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
            }

            Text {
                text: `Apply changes to ${themeSettings.currentTheme}?`
                color: Theme.var.textStrong
                font.family: Theme.var.fontSans
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Text {
                width: parent.width
                topPadding: 4
                text: `Your edits are previewing live but aren't saved yet. Apply keeps them, Discard returns to the last applied state, and Cancel keeps you on ${themeSettings.currentTheme} instead of switching to ${root.targetTheme}.`
                color: Theme.var.textLabel
                font.family: Theme.var.fontSans
                font.pixelSize: 12
                lineHeight: 18
                lineHeightMode: Text.FixedHeight
                wrapMode: Text.WordWrap
            }
        }

        Row {
            anchors.right: parent.right
            spacing: 8

            TextButton {
                text: "Cancel"
                variant: "flat"
                onClicked: root.close()
            }

            TextButton {
                text: "Discard"
                onClicked: {
                    root.close();
                    root.discardChosen();
                }
            }

            TextButton {
                text: "Apply"
                variant: "accent"
                onClicked: {
                    root.close();
                    root.applyChosen();
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: 130
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "scale"
            from: 0.94
            to: 1
            duration: 130
            easing.type: Easing.OutCubic
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            to: 0
            duration: 100
        }
    }
}
