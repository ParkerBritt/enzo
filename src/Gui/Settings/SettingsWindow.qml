import QtQuick
import QtQuick.Window
import Enzo
import "../Components"

// The settings window, one page per section listed down the left.
ScaledWindow {
    id: root

    property int currentPage: 0

    // The settings pages, each with the view-model holding its edits or null
    // when it has none.
    readonly property var pages: [
        {
            title: "Interface",
            icon: "monitor",
            source: "InterfacePage.qml",
            settings: interfaceSettings
        },
        {
            title: "Theme",
            icon: "palette",
            source: "ThemePage.qml",
            settings: themeSettings
        },
        {
            title: "Keymap",
            icon: "keyboard",
            source: "KeymapPage.qml",
            settings: null
        },
    ]

    readonly property var page: root.pages[root.currentPage]

    readonly property bool pageHasPendingChanges: root.hasPendingChanges(root.page)

    // The view-models holding edits that are not applied yet.
    readonly property var pendingSettings: root.pages.filter(page => root.hasPendingChanges(page)).map(page => page.settings)

    // Returns whether a page holds edits that are not applied yet.
    function hasPendingChanges(page) {
        return page.settings !== null && page.settings.hasPendingChanges;
    }

    // Applies or reverts every page's edits, then closes the window.
    function closeWith(action) {
        const pending = root.pendingSettings;
        for (const settings of pending)
            settings[action]();
        root.close();
    }

    onClosing: close => {
        if (root.pendingSettings.length === 0)
            return;
        close.accepted = false;
        closePrompt.open();
    }

    width: Math.round(1040 * root.uiScale)
    height: Math.round(720 * root.uiScale)
    minimumWidth: Math.round(720 * root.uiScale)
    minimumHeight: Math.round(460 * root.uiScale)
    title: "Enzo Settings"
    color: Theme.var.background

    // The list of pages.
    Rectangle {
        id: sidebar

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 7
        width: 196
        radius: Theme.var.panelRadius
        color: Theme.var.surfaceHeader
        border.color: Theme.var.border

        Column {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 2

            Repeater {
                model: root.pages

                delegate: Rectangle {
                    id: entry

                    required property int index
                    required property var modelData

                    readonly property bool current: root.currentPage === entry.index

                    width: parent.width
                    height: 28
                    radius: 8
                    color: entry.current ? Theme.var.selectedFill : hover.hovered ? Theme.var.borderSoft : "transparent"

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 9

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: entry.modelData.icon
                            size: 14
                            color: entry.current ? Theme.var.textStrong : Theme.var.textLabel
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: entry.modelData.title
                            color: entry.current ? Theme.var.textStrong : Theme.var.textLabel
                            font.family: Theme.var.fontSans
                            font.pixelSize: 12
                            font.weight: entry.current ? Font.DemiBold : Font.Medium
                        }
                    }

                    // Marks a page holding edits that are not written yet.
                    Icon {
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        visible: root.hasPendingChanges(entry.modelData)
                        name: "pencil"
                        size: 12
                        color: Theme.var.text
                    }

                    HoverHandler {
                        id: hover
                    }

                    TapHandler {
                        onTapped: root.currentPage = entry.index
                    }
                }
            }
        }
    }

    Panel {
        anchors.left: sidebar.right
        anchors.leftMargin: 7
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 7

        Loader {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: footer.top
            source: root.page.source
        }

        // The bar that applies or discards the current page's edits.
        Item {
            id: footer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 52

            Rectangle {
                anchors.fill: parent
                color: Theme.var.surfaceHeader

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1
                    color: Theme.var.borderSoft
                }
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: root.pageHasPendingChanges ? "Unsaved changes" : "No pending changes"
                color: root.pageHasPendingChanges ? Theme.var.text : Theme.var.textMuted
                font.family: Theme.var.fontSans
                font.pixelSize: 12
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                TextButton {
                    text: "Revert"
                    enabled: root.pageHasPendingChanges
                    onClicked: root.page.settings.revert()
                }

                TextButton {
                    text: "Apply"
                    variant: "accent"
                    enabled: root.pageHasPendingChanges
                    onClicked: root.page.settings.apply()
                }
            }
        }
    }

    PendingChangesPrompt {
        id: closePrompt

        heading: "Closing settings"
        question: "Apply your changes?"
        cancelOutcome: "keeps the settings open"
        onApplyChosen: root.closeWith("apply")
        onDiscardChosen: root.closeWith("revert")
    }
}
