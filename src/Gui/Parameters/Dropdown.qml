import QtQuick
import QtQuick.Controls
import "../Components"

// Picks one option, storing its token as the value.
Item {
    id: root

    required property var item

    implicitHeight: 22

    readonly property var options: item ? item.options : []
    readonly property var tokens: item ? item.optionTokens : []
    readonly property int currentIndex: item ? tokens.indexOf(item.value) : -1
    readonly property string currentText:
        (currentIndex >= 0 && currentIndex < options.length) ? options[currentIndex] : ""

    // Highlighted row, driven by hover, drag, and keyboard. Tracking the current
    // row while closed keeps the highlight steady when the list opens.
    property int activeIndex: currentIndex

    // Row under a point given in this item's coordinates, or -1 when the point
    // misses the open list. Lets a press that began on the box track and land on
    // a row as the cursor drags down into the popup.
    function rowAt(px, py) {
        if (!popup.visible || px < 0 || px > width) return -1
        const localY = py - (popup.y + popup.padding)
        const index = Math.floor(localY / 22)
        return (localY >= 0 && index < options.length) ? index : -1
    }

    function choose(index) {
        item.value = tokens[index]
        popup.close()
    }

    function step(delta) {
        const count = options.length
        if (count === 0) return
        activeIndex = Math.max(0, Math.min(count - 1, (activeIndex < 0 ? 0 : activeIndex) + delta))
    }

    Rectangle {
        id: box

        anchors.fill: parent
        radius: Theme.parameterRadius
        color: Theme.parameterBg
        border.color: Theme.parameterLine

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: chevron.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: root.currentText
            color: Theme.text
            font.family: Theme.fontUi
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Icon {
            id: chevron

            name: "chevron-down"
            size: 14
            color: Theme.muted
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            rotation: popup.visible ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        }

        MouseArea {
            anchors.fill: parent
            onPressed: {
                if (!popup.visible) popup.open()
            }
            onPositionChanged: (mouse) => {
                const index = root.rowAt(mouse.x, mouse.y)
                if (index >= 0) root.activeIndex = index
            }
            onReleased: (mouse) => {
                const index = root.rowAt(mouse.x, mouse.y)
                if (index >= 0) root.choose(index)
            }
        }
    }

    Popup {
        id: popup

        y: root.height + 2
        width: root.width
        padding: 4
        clip: true
        focus: true

        readonly property real fullHeight: Math.min(column.implicitHeight + padding * 2, 240)
        implicitHeight: fullHeight

        onClosed: root.activeIndex = root.currentIndex

        contentItem: Item {
            implicitHeight: column.implicitHeight
            focus: true

            Keys.onPressed: (event) => {
                switch (event.key) {
                case Qt.Key_Up:     root.step(-1); event.accepted = true; break
                case Qt.Key_Down:   root.step(1);  event.accepted = true; break
                case Qt.Key_Return:
                case Qt.Key_Enter:  if (root.activeIndex >= 0) root.choose(root.activeIndex); event.accepted = true; break
                case Qt.Key_Escape: popup.close(); event.accepted = true; break
                }
            }

            Rectangle {
                width: column.width
                height: 22
                radius: Theme.parameterRadius
                color: Theme.bsoft
                y: root.activeIndex * 22
                visible: root.activeIndex >= 0
                Behavior on y { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            }

            Column {
                id: column

                width: popup.availableWidth

                Repeater {
                    model: root.options

                    Rectangle {
                        readonly property bool active: index === root.activeIndex

                        width: column.width
                        height: 22
                        color: "transparent"

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData
                            color: (active || index === root.currentIndex) ? Theme.text : Theme.label
                            font.family: Theme.fontUi
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        HoverHandler {
                            onHoveredChanged: if (hovered) root.activeIndex = index
                        }
                        TapHandler { onTapped: root.choose(index) }
                    }
                }
            }
        }

        background: Rectangle {
            radius: Theme.parameterRadius
            color: Theme.parameterBg
            border.color: Theme.parameterLine
        }

        enter: Transition {
            NumberAnimation { property: "height"; from: 0; to: popup.fullHeight; duration: 160; easing.type: Easing.OutCubic }
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 110 }
        }
        exit: Transition {
            NumberAnimation { property: "height"; to: 0; duration: 120; easing.type: Easing.InCubic }
            NumberAnimation { property: "opacity"; to: 0; duration: 120 }
        }
    }
}
