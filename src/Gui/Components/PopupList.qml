import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Enzo
import "../Utils.js" as Utils

// Animated floating list shared by the menus and the parameter pickers. Owns the
// unroll reveal, the gliding highlight and keyboard travel, while a consumer fills
// in the row model and a delegate for how a row looks.
//
// e.g.
//   PopupList {
//       model: entries
//       delegate: MyRow { highlighted: index === id.highlightedIndex }
//       onActivated: (index) => entries[index].run()
//   }
Popup {
    id: root

    // Rows to show. Each element is opaque to the base and handed to the delegate
    // as modelData.
    property var model: []

    // Visual height of one row.
    property int rowHeight: 26

    // Component drawing a single row. The Repeater hands it index and modelData,
    // and it reads the highlight by comparing index to highlightedIndex.
    required property Component delegate

    // Whether a row can take the highlight. A consumer with separators or disabled
    // rows narrows this so navigation and hover skip them.
    property var canHighlight: (index) => true

    // Row currently under the highlight.
    property int highlightedIndex: 0

    // Row chosen by click or Enter.
    signal activated(int index)

    readonly property int count: model ? model.length : 0
    readonly property real fullHeight: count * rowHeight + padding * 2

    padding: 4
    focus: true
    clip: true
    implicitWidth: 180
    implicitHeight: fullHeight

    // Moves the highlight in a direction, skipping rows that cannot take it.
    function step(delta) {
        let next = highlightedIndex + delta
        while (next >= 0 && next < count)
        {
            if (canHighlight(next)) { highlightedIndex = next; return }
            next += delta
        }
    }

    function activate() {
        if (highlightedIndex >= 0 && highlightedIndex < count && canHighlight(highlightedIndex))
            root.activated(highlightedIndex)
    }

    // Row under a y in content coordinates, or -1 when the point misses a row.
    function rowAt(localY) {
        const index = Math.floor(localY / rowHeight)
        return (index >= 0 && index < count) ? index : -1
    }

    background: Rectangle {
        radius: 8
        color: Theme.var.surface
        border.color: Theme.var.border
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.7
            shadowOpacity: 0.3
            shadowHorizontalOffset: 2
            shadowVerticalOffset: 2
        }
    }

    contentItem: Item {
        implicitHeight: root.count * root.rowHeight
        focus: true

        Keys.onUpPressed: root.step(-1)
        Keys.onDownPressed: root.step(1)
        Keys.onReturnPressed: root.activate()
        Keys.onEnterPressed: root.activate()
        Keys.onEscapePressed: root.close()

        // Gliding highlight drawn behind the rows.
        Rectangle {
            width: parent.width
            height: root.rowHeight
            radius: 6
            color: Theme.var.accentDim
            y: root.highlightedIndex * root.rowHeight
            visible: root.highlightedIndex >= 0 && root.canHighlight(root.highlightedIndex)
            Behavior on y { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
        }

        Column {
            width: parent.width
            Repeater {
                model: root.model
                delegate: root.delegate
            }
        }

        // One area maps the cursor to a row so hover sets the highlight and a
        // click chooses it, keeping pointer and keyboard travel on the same path.
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onPositionChanged: (mouse) => {
                const index = root.rowAt(mouse.y)
                if (index >= 0 && root.canHighlight(index)) root.highlightedIndex = index
            }
            onClicked: (mouse) => {
                const index = root.rowAt(mouse.y)
                if (index >= 0 && root.canHighlight(index)) root.activated(index)
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "height"; from: 0; to: root.fullHeight; duration: 160; easing.type: Easing.OutCubic }
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 110 }
    }
    exit: Transition {
        NumberAnimation { property: "height"; to: 0; duration: 120; easing.type: Easing.InCubic }
        NumberAnimation { property: "opacity"; to: 0; duration: 120 }
    }
}
