import QtQuick
import Enzo
import "../Components"

// One palette value, named on the left and edited on the right.
Item {
    id: root

    property var entry
    property bool editable: false

    // The swatch was clicked, asking the page for the colour picker.
    signal pickerRequested

    // The value the interface is drawn in.
    readonly property var value: Theme.var[root.entry.name]

    readonly property bool modified: themeSettings.modified.includes(root.entry.name)

    readonly property bool isColor: root.entry.kind === "color"
    readonly property bool isFont: root.entry.kind === "font"
    readonly property bool isNumber: root.entry.kind === "number"

    // The field colours for an editable theme and for a locked one.
    readonly property color fieldColor: root.editable ? Theme.var.fieldSurface : "transparent"
    readonly property color fieldBorderColor: root.editable ? Theme.var.border : Theme.var.borderSoft
    readonly property color fieldTextColor: root.editable ? Theme.var.text : Theme.var.textLabel

    implicitHeight: 28

    function commitHex(text) {
        const hex = text.trim();
        if (/^#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$/.test(hex))
            themeSettings.setValue(root.entry.name, hex);
    }

    Row {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        // The colour itself, which opens the picker.
        Rectangle {
            visible: root.isColor
            anchors.verticalCenter: parent.verticalCenter
            width: 18
            height: 18
            radius: 5
            color: root.value
            border.color: Qt.rgba(1, 1, 1, 0.16)

            MouseArea {
                anchors.fill: parent
                enabled: root.editable
                cursorShape: Qt.PointingHandCursor
                onClicked: root.pickerRequested()
            }
        }

        // The stand-in marker a font or a number carries in the swatch's place.
        Rectangle {
            visible: !root.isColor
            anchors.verticalCenter: parent.verticalCenter
            width: 18
            height: 18
            radius: 5
            color: Theme.var.fieldSurface
            border.color: Theme.var.border

            Text {
                anchors.centerIn: parent
                visible: root.isFont
                text: "Aa"
                color: Theme.var.textLabel
                font.family: Theme.var.fontSans
                font.pixelSize: 10
                font.weight: Font.Bold
            }

            Icon {
                anchors.centerIn: parent
                visible: root.isNumber
                name: "rotate-ccw-square"
                size: 11
                color: Theme.var.textLabel
            }
        }

        Row {
            width: parent.width - 18 - editor.width - 2 * parent.spacing
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.entry.label
                color: Theme.var.text
                font.family: Theme.var.fontSans
                font.pixelSize: 12
                font.weight: Font.Medium
            }

            // Marks a value edited since the theme was last written.
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.modified
                name: "pencil"
                size: 11
                color: Theme.var.textLabel
            }
        }

        Item {
            id: editor

            anchors.verticalCenter: parent.verticalCenter
            width: root.isNumber ? 122 : root.isFont ? 124 : 92
            height: 24

            // The colour as text, which can be typed over.
            Rectangle {
                anchors.fill: parent
                visible: root.isColor
                radius: 7
                color: root.fieldColor
                border.color: root.fieldBorderColor

                TextInput {
                    id: hex

                    anchors.fill: parent
                    anchors.leftMargin: 9
                    anchors.rightMargin: 9
                    verticalAlignment: Text.AlignVCenter
                    text: root.isColor ? String(root.value) : ""
                    color: root.fieldTextColor
                    font.family: Theme.var.fontMono
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    readOnly: !root.editable
                    selectByMouse: root.editable
                    selectionColor: Theme.var.accent

                    onEditingFinished: root.commitHex(text)
                    Keys.onEscapePressed: {
                        text = String(root.value);
                        focus = false;
                    }
                }
            }

            Dropdown {
                anchors.fill: parent
                visible: root.isFont
                enabled: root.editable
                labels: themeSettings.fontFamilies
                currentIndex: root.isFont ? themeSettings.fontFamilies.indexOf(root.value) : -1
                listWidth: 160
                onActivated: index => themeSettings.setValue(root.entry.name, themeSettings.fontFamilies[index])
            }

            Row {
                anchors.fill: parent
                visible: root.isNumber
                spacing: 10

                NumberTrack {
                    id: track

                    anchors.verticalCenter: parent.verticalCenter
                    enabled: root.editable
                    value: root.isNumber ? root.value : 0
                    onMoved: value => themeSettings.setValue(root.entry.name, value)
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 52
                    height: 24
                    radius: 7
                    color: root.fieldColor
                    border.color: root.fieldBorderColor

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 9
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.isNumber ? root.value : ""
                        color: root.fieldTextColor
                        font.family: Theme.var.fontMono
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 9
                        anchors.verticalCenter: parent.verticalCenter
                        text: "px"
                        color: Theme.var.textMuted
                        font.family: Theme.var.fontMono
                        font.pixelSize: 12
                    }
                }
            }
        }
    }
}
