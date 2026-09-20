import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Enzo
import "../Components"
import "../Utils.js" as Utils

// The popup a palette colour is mixed in.
Popup {
    id: root

    // The palette entry being mixed, `{ name, label, kind }`.
    property var entry: null

    property real hue: 0
    property real saturation: 0
    property real brightness: 0

    // The colour the picker opened on, to compare the mix against.
    property color previousColor: "transparent"

    readonly property color color: Qt.hsva(root.hue, root.saturation, root.brightness, 1)

    function show(entry, x, y) {
        root.entry = entry;
        root.x = x;
        root.y = y;

        const color = Theme.var[entry.name];
        root.hue = Math.max(color.hsvHue, 0);
        root.saturation = color.hsvSaturation;
        root.brightness = color.hsvValue;
        root.previousColor = color;
        root.open();
    }

    function commit() {
        themeSettings.setValue(root.entry.name, root.color);
    }

    padding: 14
    width: 264
    modal: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

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
        spacing: 12

        Item {
            width: parent.width
            height: 28

            Column {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: root.entry ? root.entry.label : ""
                    color: Theme.var.textStrong
                    font.family: Theme.var.fontSans
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Text {
                    text: root.entry ? root.entry.name : ""
                    color: Theme.var.textMuted
                    font.family: Theme.var.fontMono
                    font.pixelSize: 11
                }
            }

            IconButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 24
                height: 24
                name: "x"
                iconSize: 12
                onClicked: root.close()
            }
        }

        // The square the saturation and brightness are picked from.
        Rectangle {
            id: plane

            width: parent.width
            height: 148
            radius: 8
            border.color: Theme.var.border
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0
                    color: "#ffffff"
                }
                GradientStop {
                    position: 1
                    color: Qt.hsva(root.hue, 1, 1, 1)
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: "transparent"
                    }
                    GradientStop {
                        position: 1
                        color: "#000000"
                    }
                }
            }

            Rectangle {
                x: root.saturation * plane.width - width / 2
                y: (1 - root.brightness) * plane.height - height / 2
                width: 14
                height: 14
                radius: 7
                color: "transparent"
                border.color: "#ffffff"
                border.width: 2
            }

            MouseArea {
                anchors.fill: parent

                function mix(mouse) {
                    root.saturation = Utils.clamp(mouse.x / plane.width, 0, 1);
                    root.brightness = 1 - Utils.clamp(mouse.y / plane.height, 0, 1);
                    root.commit();
                }

                onPressed: mouse => mix(mouse)
                onPositionChanged: mouse => mix(mouse)
            }
        }

        Rectangle {
            id: hueStrip

            width: parent.width
            height: 12
            radius: 6
            border.color: Theme.var.border
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0
                    color: "#ff0000"
                }
                GradientStop {
                    position: 0.17
                    color: "#ffff00"
                }
                GradientStop {
                    position: 0.33
                    color: "#00ff00"
                }
                GradientStop {
                    position: 0.5
                    color: "#00ffff"
                }
                GradientStop {
                    position: 0.67
                    color: "#0000ff"
                }
                GradientStop {
                    position: 0.83
                    color: "#ff00ff"
                }
                GradientStop {
                    position: 1
                    color: "#ff0000"
                }
            }

            Rectangle {
                x: root.hue * hueStrip.width - width / 2
                anchors.verticalCenter: parent.verticalCenter
                width: 12
                height: 12
                radius: 6
                color: "transparent"
                border.color: "#ffffff"
                border.width: 2
            }

            MouseArea {
                anchors.fill: parent
                anchors.margins: -6

                function mix(mouse) {
                    root.hue = Utils.clamp((mouse.x - 6) / hueStrip.width, 0, 1);
                    root.commit();
                }

                onPressed: mouse => mix(mouse)
                onPositionChanged: mouse => mix(mouse)
            }
        }

        Row {
            width: parent.width
            spacing: 8

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "HEX"
                color: Theme.var.textMuted
                font.family: Theme.var.fontMono
                font.pixelSize: 11
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: String(root.color)
                color: Theme.var.text
                font.family: Theme.var.fontMono
                font.pixelSize: 12
                font.weight: Font.Medium
            }
        }

        // The colour the picker opened on and the current mix.
        Row {
            width: parent.width
            spacing: 8

            ColorCompare {
                label: "WAS"
                color: root.previousColor
            }

            ColorCompare {
                label: "NOW"
                color: root.color
            }
        }
    }
}
