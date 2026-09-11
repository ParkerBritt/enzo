import QtQuick
import QtQuick.Controls
import Enzo

// A container that scrolls its contents with the wheel and a slim scrollbar.
Flickable {
    id: area

    default property alias content: holder.data

    // Pixels the content travels per unit of wheel rotation.
    property real wheelStep: 0.8
    // Milliseconds the content takes to settle after a wheel move.
    property int glideDuration: 300
    property real barWidth: 4

    // Where the wheel is taking the content. Notches in quick succession add up
    // from here rather than from the position the glide has reached.
    property real wheelTargetY: 0

    // The furthest down the content can sit with its bottom still in view.
    readonly property real maxContentY: Math.max(0, contentHeight - height)

    clip: true
    interactive: false
    contentWidth: width
    contentHeight: holder.childrenRect.height

    // Glides the content to where the wheel sent it. Off while the scrollbar
    // handle is dragged, so the handle keeps up with the cursor.
    Behavior on contentY {
        enabled: !bar.pressed
        NumberAnimation {
            duration: area.glideDuration
            easing.type: Easing.OutCubic
        }
    }

    onContentYChanged: if (bar.pressed) wheelTargetY = contentY

    // Pulls the view and the wheel target back inside the content bounds.
    function settleInBounds() {
        returnToBounds();
        wheelTargetY = Math.min(wheelTargetY, maxContentY);
    }

    onHeightChanged: settleInBounds()
    onContentHeightChanged: settleInBounds()

    // Catches the wheel anywhere over the content. Sits below the content so a
    // press reaches the control under the cursor.
    MouseArea {
        // Sized rather than anchored, since anchoring to the content would make
        // contentHeight depend on itself.
        width: area.width
        height: Math.max(area.height, area.contentHeight)

        onWheel: wheel => {
            const step = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y * area.wheelStep;
            area.wheelTargetY = Math.max(0, Math.min(area.maxContentY, area.wheelTargetY - step));
            area.contentY = area.wheelTargetY;
        }
    }

    Item {
        id: holder
        width: area.width
        height: area.contentHeight
    }

    ScrollBar.vertical: ScrollBar {
        id: bar

        policy: ScrollBar.AsNeeded
        width: area.barWidth

        contentItem: Rectangle {
            implicitWidth: area.barWidth
            radius: width / 2
            color: bar.pressed ? Theme.var.textMuted : Theme.var.textFaint
        }

        background: null
    }
}
