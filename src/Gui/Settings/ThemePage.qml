import QtQuick
import Enzo
import "../Components"

// The page that picks the theme and edits its palette.
Item {
    id: root

    property bool renaming: false

    // Switches theme, asking first when the current theme holds unwritten edits.
    function requestTheme(name) {
        if (!themeSettings.hasPendingChanges) {
            themeSettings.selectTheme(name);
            return;
        }
        prompt.ask(name);
    }

    // Opens the picker under a colour row, kept inside the page.
    function openPicker(row) {
        const corner = row.mapToItem(root, 0, row.height + 6);
        const x = Math.min(corner.x, root.width - picker.width - 20);
        const y = Math.min(corner.y, root.height - picker.height - 20);
        picker.show(row.entry, x, y);
    }

    function startRename() {
        renaming = true;
        nameField.text = themeSettings.currentTheme;
        nameField.selectAll();
        nameField.forceActiveFocus();
    }

    function finishRename() {
        if (!renaming)
            return;
        renaming = false;
        themeSettings.renameTheme(nameField.text.trim());
    }

    // The row naming the theme in use, with what can be done to it.
    Item {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 52

        ThemeSelector {
            id: selector

            visible: !root.renaming
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            onThemeChosen: name => root.requestTheme(name)
        }

        // Takes the selector's place while the theme is being renamed.
        Rectangle {
            visible: root.renaming
            anchors.fill: selector
            radius: 8
            color: Theme.var.fieldSurface
            border.color: Theme.var.accentLine

            TextInput {
                id: nameField

                anchors.fill: parent
                anchors.leftMargin: 9
                anchors.rightMargin: 9
                verticalAlignment: Text.AlignVCenter
                color: Theme.var.textStrong
                font.family: Theme.var.fontSans
                font.pixelSize: 12
                font.weight: Font.DemiBold
                selectByMouse: true
                selectionColor: Theme.var.accent

                onEditingFinished: root.finishRename()
                Keys.onEscapePressed: {
                    root.renaming = false;
                    focus = false;
                }
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            TextButton {
                text: "Duplicate"
                onClicked: themeSettings.duplicateTheme()
            }

            TextButton {
                text: "Rename"
                enabled: themeSettings.editable
                onClicked: root.startRename()
            }

            TextButton {
                text: "Delete"
                variant: "danger"
                enabled: themeSettings.editable
                onClicked: themeSettings.deleteTheme()
            }

            TextButton {
                text: "Open theme folder"
                variant: "flat"
                icon: "folder"
                onClicked: themeSettings.openThemeFolder()
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.var.borderSoft
        }
    }

    // Says why a shipped theme cannot be edited.
    Rectangle {
        id: banner

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        height: visible ? 36 : 0
        visible: !themeSettings.editable
        color: Theme.var.surfaceHeader

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: "lock"
                size: 12
                color: Theme.var.textLabel
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Shipped themes are read only. Duplicate this theme to start editing."
                color: Theme.var.textLabel
                font.family: Theme.var.fontSans
                font.pixelSize: 12
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.var.borderSoft
        }
    }

    ScrollArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: banner.bottom
        anchors.bottom: parent.bottom

        Column {
            x: 20
            width: parent.width - 40
            spacing: 14

            // Counts the inset towards the height the scroll area reads off
            // the content.
            topPadding: 14
            bottomPadding: 24

            Repeater {
                model: themeSettings.sections

                delegate: Column {
                    id: section

                    required property var modelData

                    width: parent.width

                    Row {
                        width: parent.width
                        height: 28
                        spacing: 10

                        Text {
                            id: heading

                            anchors.verticalCenter: parent.verticalCenter
                            text: section.modelData.title
                            color: Theme.var.textLabel
                            font.family: Theme.var.fontSans
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            font.letterSpacing: 1.2
                            font.capitalization: Font.AllUppercase
                        }

                        Rectangle {
                            width: parent.width - heading.width - parent.spacing
                            height: 1
                            anchors.verticalCenter: parent.verticalCenter
                            color: Theme.var.borderSoft
                        }
                    }

                    Grid {
                        width: parent.width
                        columns: 2
                        columnSpacing: 28

                        Repeater {
                            model: section.modelData.entries

                            delegate: PaletteRow {
                                id: paletteRow

                                required property var modelData

                                width: (section.width - 28) / 2
                                entry: modelData
                                editable: themeSettings.editable
                                onPickerRequested: root.openPicker(paletteRow)
                            }
                        }
                    }
                }
            }
        }
    }

    ColorPicker {
        id: picker
    }

    SwitchThemePrompt {
        id: prompt

        onApplyChosen: {
            themeSettings.apply();
            themeSettings.selectTheme(prompt.targetTheme);
        }
        onDiscardChosen: {
            themeSettings.revert();
            themeSettings.selectTheme(prompt.targetTheme);
        }
    }
}
