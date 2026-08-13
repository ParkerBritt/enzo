import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Enzo
import "../Utils.js" as Utils

// Animated floating list shared by the menus and the parameter pickers. Owns the
// unroll reveal, the gliding highlight, keyboard travel and submenu nesting,
// while a consumer fills in the row model and a delegate for how a row looks.
//
// A row is opaque to the base except for a few optional fields it recognises
// by convention: `enabled` and `separator` narrow which rows can take the
// highlight, `children` opens a nested PopupList of further rows beside it,
// and `action` is what runLeafAction runs when a plain row (no children) is
// chosen.
//
// e.g.
//   PopupList {
//       model: entries
//       delegate: MyRow { highlighted: index === id.highlightedIndex }
//       onActivated: (index) => root.runLeafAction(index)
//   }
Popup {
    id: root

    // Rows to show. Each element is opaque to the base and handed to the delegate
    // as modelData.
    property var model: []

    // Visual height of one row.
    property int rowHeight: 26

    // Height of a given row, defaulting to the uniform rowHeight. A consumer
    // with a shorter separator row overrides this per index.
    property var rowHeightAt: (index) => rowHeight

    // Component drawing a single row. The Repeater hands it index and modelData,
    // and it reads the highlight by comparing index to highlightedIndex.
    required property Component delegate

    // Whether a row can take the highlight, skipping a disabled row or a
    // separator by default. A consumer with its own notion of skippable rows
    // narrows this further.
    property var canHighlight: (index) => {
        const entry = root.model[index]
        return entry !== undefined && entry.enabled !== false && !entry.separator
    }

    // Row currently under the highlight.
    property int highlightedIndex: 0

    // Row chosen by click or Enter, for a row without children. A row with
    // children never reaches this signal since choosing it just opens its
    // submenu, which is already showing once it is highlighted.
    signal activated(int index)

    // Runs a row's own action, if it has one, then fires leafChosen. The
    // default way for a consumer to answer `activated` when its rows follow
    // the `action` convention.
    function runLeafAction(index) {
        const entry = root.model[index]
        if (entry?.action)
            entry.action()
        root.leafChosen()
    }

    // Row whose children are open as a nested submenu, or -1 when none. A row
    // opts in by carrying a non-empty `children` array of further rows.
    readonly property int submenuIndex: root.model[root.highlightedIndex]?.children ? root.highlightedIndex : -1

    // Width of a nested submenu, defaulting to this list's own width.
    property int submenuWidth: implicitWidth

    // Fires when a leaf row is chosen at any depth, so every open level of a
    // nested menu closes together instead of leaving ancestor levels stranded.
    signal leafChosen
    onLeafChosen: root.close()

    // Content shown above the row list, outside the highlight and keyboard
    // travel. A consumer that sets this must also set headerHeight to its
    // rendered height, since that height is needed before the header itself
    // has laid out.
    property Component header: null
    property int headerHeight: 0

    readonly property int count: model ? model.length : 0
    readonly property real rowsHeight: {
        let total = 0
        for (let i = 0; i < count; i++) total += rowHeightAt(i)
        return total
    }
    readonly property real fullHeight: rowsHeight + headerHeight + padding * 2

    // Returns the top y of a row in content coordinates, the sum of every row before it.
    function rowTop(index) {
        let y = 0
        for (let i = 0; i < index; i++) y += rowHeightAt(i)
        return y
    }

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
        root.tryActivate(root.highlightedIndex)
    }

    // Fires activated for a plain row, or does nothing for a row with
    // children since highlighting it already opened its submenu.
    function tryActivate(index) {
        if (index < 0 || index >= count || !canHighlight(index))
            return
        if (root.model[index]?.children)
            return
        root.activated(index)
    }

    // Row under a y in content coordinates, or -1 when the point misses a row.
    function rowAt(localY) {
        let y = 0
        for (let i = 0; i < count; i++) {
            const h = rowHeightAt(i)
            if (localY >= y && localY < y + h) return i
            y += h
        }
        return -1
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
        implicitHeight: root.rowsHeight + root.headerHeight
        focus: true

        Keys.onUpPressed: root.step(-1)
        Keys.onDownPressed: root.step(1)
        Keys.onReturnPressed: root.activate()
        Keys.onEnterPressed: root.activate()
        Keys.onEscapePressed: root.close()

        Loader {
            width: parent.width
            height: root.headerHeight
            z: 2
            active: root.header !== null
            sourceComponent: root.header
        }

        // Gliding highlight drawn behind the rows.
        Rectangle {
            width: parent.width
            height: root.rowHeightAt(root.highlightedIndex)
            radius: 6
            color: Theme.popup.highlightColor
            y: root.headerHeight + root.rowTop(root.highlightedIndex)
            visible: root.highlightedIndex >= 0 && root.canHighlight(root.highlightedIndex)
            Behavior on y { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
        }

        // Above the cursor area below, so a delegate can carry its own button.
        Column {
            y: root.headerHeight
            width: parent.width
            z: 1
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
                const index = root.rowAt(mouse.y - root.headerHeight)
                if (index >= 0 && root.canHighlight(index)) root.highlightedIndex = index
            }
            onClicked: (mouse) => {
                const index = root.rowAt(mouse.y - root.headerHeight)
                if (index >= 0) root.tryActivate(index)
            }
        }

        // Nested submenu, loaded fresh each time the highlight lands on a row
        // with children so its entries reflect whatever is current. Wired to
        // run its own rows' actions the same way an author-declared PopupList
        // would, since nothing else declares this instance.
        //
        // A component cannot declare itself as a child inline, so it loads
        // itself by file instead, through setSource so its required
        // properties are supplied at creation rather than after.
        Loader {
            id: submenuLoader

            active: root.submenuIndex >= 0
            onActiveChanged: if (active) submenuLoader.setSource("PopupList.qml", {
                model: Qt.binding(() => root.model[root.submenuIndex]?.children ?? []),
                delegate: root.delegate,
                rowHeight: root.rowHeight,
                rowHeightAt: root.rowHeightAt,
                implicitWidth: root.submenuWidth,
                closePolicy: root.closePolicy
            })
            onLoaded: {
                item.parent = root.contentItem
                item.x = Qt.binding(() => root.contentItem.width + root.padding)
                item.y = Qt.binding(() => root.headerHeight + root.rowTop(root.submenuIndex) - root.padding)
                item.activated.connect(item.runLeafAction)
                item.leafChosen.connect(root.leafChosen)
                item.open()
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
