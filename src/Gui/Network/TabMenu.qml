import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Enzo
import "../Utils.js" as Utils

// Search menu that creates a node at the cursor. The search field holds focus so
// typing filters the list while the arrow keys, Return, Tab and Escape navigate
// and close without the focus ever leaving the field.
Popup {
    id: root

    // Every operator type to choose from, each a {label, name} map.
    property var nodeTypes: []
    // Emitted with the chosen operator's internal name.
    signal nodeTypeChosen(string name)

    width: 300
    padding: 8
    focus: true
    background: background

    // Operators matching the current query and the row under the highlight.
    property var matches: []
    property int highlighted: 0
    readonly property int rowHeight: 28
    readonly property int maxVisibleRows: 10

    function refilter() {
        const needle = search.text.toLowerCase();
        matches = nodeTypes.filter(type => type.label.toLowerCase().includes(needle));
        highlighted = 0;
    }

    function moveHighlight(delta) {
        highlighted = Utils.clamp(highlighted + delta, 0, matches.length - 1);
        list.positionViewAtIndex(highlighted, ListView.Contain);
    }

    function activate() {
        if (matches.length === 0)
            return;
        root.nodeTypeChosen(matches[highlighted].name);
        root.close();
    }

    onAboutToShow: {
        search.text = "";
        refilter();
    }
    onOpened: search.forceActiveFocus()

    Rectangle {
        id: background
        color: Theme.var.surface
        border.color: Theme.var.border
        radius: 8
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

    contentItem: Column {
        spacing: 8

        TextField {
            id: search
            width: parent.width
            placeholderText: "Search nodes"
            color: Theme.var.text
            placeholderTextColor: Theme.var.textMuted
            font.family: Theme.var.fontSans
            onTextChanged: root.refilter()
            background: Rectangle {
                radius: 7
                color: Theme.var.fieldSurface
                border.color: search.activeFocus ? Theme.tabMenu.focusBorderColor : Theme.var.fieldBorder
            }

            // Navigation keys steer the list, every other key edits the text.
            Keys.onUpPressed: root.moveHighlight(-1)
            Keys.onDownPressed: root.moveHighlight(1)
            Keys.onReturnPressed: root.activate()
            Keys.onEnterPressed: root.activate()
            Keys.onEscapePressed: root.close()
            Keys.onTabPressed: root.close()
        }

        ListView {
            id: list
            width: parent.width
            height: Math.min(root.matches.length, root.maxVisibleRows) * root.rowHeight
            clip: true
            model: root.matches
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: ListView.view.width
                height: root.rowHeight
                radius: 6
                color: index === root.highlighted ? Theme.tabMenu.highlightColor : "transparent"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 12
                    text: modelData.label
                    color: Theme.var.text
                    font.family: Theme.var.fontSans
                    font.pixelSize: 13
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: root.highlighted = index
                    onClicked: root.activate()
                }
            }
        }
    }
}
