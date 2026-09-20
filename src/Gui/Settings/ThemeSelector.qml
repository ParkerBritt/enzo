import QtQuick
import Enzo
import "../Components"

// The field naming the theme in use, opening the list of themes to choose from.
Item {
    id: root

    // The theme a row was chosen for.
    signal themeChosen(string name)

    readonly property var rows: {
        const shipped = themeSettings.themes.filter(theme => !theme.editable);
        const mine = themeSettings.themes.filter(theme => theme.editable);
        const rows = [{
                heading: "Shipped",
                separator: true
            }].concat(shipped);
        if (mine.length === 0)
            return rows;
        return rows.concat([{
                    heading: "Your themes",
                    separator: true
                }], mine);
    }

    readonly property var currentEntry: themeSettings.themes.find(theme => theme.name === themeSettings.currentTheme)

    implicitWidth: 232
    implicitHeight: 30

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Theme.var.fieldSurface
        border.color: themeSettings.hasPendingChanges ? Theme.var.accentLine : Theme.var.fieldBorder

        Row {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 9
            anchors.rightMargin: 9
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            ThemeSwatch {
                anchors.verticalCenter: parent.verticalCenter
                colors: root.currentEntry ? root.currentEntry.swatch : []
            }

            Text {
                width: parent.width - 26 - 12 - 2 * parent.spacing
                anchors.verticalCenter: parent.verticalCenter
                text: themeSettings.currentTheme
                color: Theme.var.textStrong
                font.family: Theme.var.fontSans
                font.pixelSize: 12
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: "chevron-down"
                size: 12
                color: Theme.var.textLabel
                rotation: list.visible ? 180 : 0

                Behavior on rotation {
                    NumberAnimation {
                        duration: 150
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: if (!list.visible)
            list.open()
    }

    PopupList {
        id: list

        y: root.height + 4
        implicitWidth: 248
        rowHeight: 30
        rowHeightAt: index => root.rows[index].separator ? 22 : 30
        model: root.rows

        onAboutToShow: highlightedIndex = root.rows.findIndex(row => row.name === themeSettings.currentTheme)
        onActivated: index => {
            root.themeChosen(root.rows[index].name);
            list.close();
        }

        delegate: Item {
            id: row

            required property int index
            required property var modelData

            readonly property bool current: row.modelData.name === themeSettings.currentTheme

            width: list.availableWidth
            height: list.rowHeightAt(row.index)

            // The heading above a group of themes.
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                visible: row.modelData.separator === true
                text: row.modelData.heading ?? ""
                color: Theme.var.textMuted
                font.family: Theme.var.fontSans
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                visible: row.modelData.separator !== true
                spacing: 10

                ThemeSwatch {
                    anchors.verticalCenter: parent.verticalCenter
                    colors: row.modelData.swatch ?? []
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.name ?? ""
                    color: row.current ? Theme.var.textStrong : Theme.var.text
                    font.family: Theme.var.fontSans
                    font.pixelSize: 12
                    font.weight: row.current ? Font.DemiBold : Font.Medium
                }
            }

            Icon {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                visible: row.current
                name: "check"
                size: 12
                color: Theme.var.text
            }
        }
    }
}
