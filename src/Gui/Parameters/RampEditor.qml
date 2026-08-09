import QtQuick
import Enzo
import "../Components"

// Curve editor for a ramp parameter.
Column {
    id: root

    required property var item

    property var points: item ? item.value : []
    property int selectedIndex: 0

    // Inset keeping the end handles inside the plot frame.
    readonly property real plotInset: 10
    readonly property real handleSize: 9
    readonly property real grabRadius: 14

    // Preset curve shapes. Choosing one is not wired up yet.
    readonly property var presetLabels: ["Linear Ramp", "Ease In", "Ease Out", "Bell Curve"]

    // Point indices in position order, since instances are stored unsorted.
    readonly property var pointOrder: [...points.keys()].sort((a, b) => points[a].position - points[b].position)

    // Place of the selected point in sorted order, for the identity label.
    readonly property int selectedRank: pointOrder.indexOf(selectedIndex) + 1

    spacing: 10

    // An undo or expression edit changes the instances behind the binding.
    Connections {
        target: root.item
        function onValueChanged() {
            root.points = root.item.value;
            root.selectedIndex = Math.min(root.selectedIndex, root.points.length - 1);
        }
    }

    // Walks the selection along the curve, wrapping past either end.
    function stepSelection(delta) {
        const rank = pointOrder.indexOf(selectedIndex);
        if (rank < 0)
            return;
        selectedIndex = pointOrder[(rank + delta + pointOrder.length) % pointOrder.length];
    }

    function commitPoints(newPoints) {
        item.beginEdit();
        item.value = newPoints;
        item.commitEdit();
    }

    function handleCenter(point) {
        return Qt.point(plotInset + point.position * curve.width, plotInset + (1 - point.value) * curve.height);
    }

    // Index of the handle nearest a plot position within grab range, or -1.
    function handleAt(x, y) {
        let best = -1;
        let bestDistance = grabRadius;
        for (let i = 0; i < points.length; ++i) {
            const center = handleCenter(points[i]);
            const distance = Math.hypot(x - center.x, y - center.y);
            if (distance < bestDistance) {
                best = i;
                bestDistance = distance;
            }
        }
        return best;
    }

    function moveSelected(x, y) {
        const clamp = v => Math.max(0, Math.min(1, v));
        const moved = points;
        moved[selectedIndex].position = clamp((x - plotInset) / curve.width);
        moved[selectedIndex].value = clamp(1 - (y - plotInset) / curve.height);
        item.value = moved;
    }

    // Splits the segment beside the selected point and selects the new point.
    function addPoint() {
        const selected = points[selectedIndex];
        if (!selected)
            return;

        // The neighbour is the next point to the right, or to the left at the end.
        let neighbour = null;
        for (const point of points)
            if (point.position > selected.position && (!neighbour || point.position < neighbour.position))
                neighbour = point;
        if (!neighbour)
            for (const point of points)
                if (point.position < selected.position && (!neighbour || point.position > neighbour.position))
                    neighbour = point;
        if (!neighbour)
            return;

        // The split keeps the segment shape by inheriting the left point's interp.
        const leftInterp = neighbour.position > selected.position ? selected.interp : neighbour.interp;
        const added = points;
        added.push({
            position: (selected.position + neighbour.position) / 2,
            value: (selected.value + neighbour.value) / 2,
            interp: leftInterp
        });
        commitPoints(added);
        selectedIndex = points.length - 1;
    }

    // The curve always keeps at least two points.
    function deleteSelected() {
        if (points.length <= 2)
            return;
        const remaining = points;
        remaining.splice(selectedIndex, 1);
        commitPoints(remaining);
    }

    function flipHorizontal() {
        const sorted = [...points].sort((a, b) => a.position - b.position);
        // A point's interp governs the segment on its right, so mirroring hands
        // each segment's interp to the point now on its left.
        commitPoints(sorted.map((point, i) => ({
                    position: 1 - point.position,
                    value: point.value,
                    interp: sorted[Math.max(i - 1, 0)].interp
                })));
    }

    function flipVertical() {
        // Mirrors across the visible band, which spans at least zero to one.
        let low = 0;
        let high = 1;
        for (const point of points) {
            low = Math.min(low, point.value);
            high = Math.max(high, point.value);
        }
        commitPoints(points.map(point => ({
                    position: point.position,
                    value: low + high - point.value,
                    interp: point.interp
                })));
    }

    Text {
        text: root.item ? root.item.label : ""
        color: Theme.var.textStrong
        font.family: Theme.var.fontSans
        font.pixelSize: 14
        font.weight: Font.DemiBold
    }

    // Point navigation on the left, presets and the point and curve ops on the right.
    Item {
        width: root.width
        height: 30

        Stepper {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            label: root.selectedRank
            detail: " / " + root.points.length
            onPrevious: root.stepSelection(-1)
            onNext: root.stepSelection(1)
        }

        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            Dropdown {
                icon: "swatch-book"
                labels: root.presetLabels
            }
            IconButton {
                variant: "field"
                name: "flip-horizontal-2"
                onClicked: root.flipHorizontal()
            }
            IconButton {
                variant: "field"
                name: "flip-vertical-2"
                onClicked: root.flipVertical()
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 1
                height: 16
                color: Theme.var.borderSoft
            }
            IconButton {
                variant: "accent"
                name: "plus"
                onClicked: root.addPoint()
            }
            IconButton {
                variant: "field"
                name: "trash-2"
                iconColor: Theme.var.danger
                enabled: root.points.length > 2
                onClicked: root.deleteSelected()
            }
        }
    }

    Rectangle {
        id: plot

        width: root.width
        height: 132
        radius: Theme.parameter.borderRadius
        color: Theme.ramp.plotColor
        border.color: Theme.ramp.lineColor

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) {
                root.deleteSelected();
                event.accepted = true;
            }
        }

        RampCurveItem {
            id: curve

            anchors.fill: parent
            anchors.margins: root.plotInset
            points: root.points
            curveColor: Theme.ramp.curveColor
            fillColor: Theme.ramp.fillColor
            curveWidth: 2
        }

        MouseArea {
            id: plotMouse

            anchors.fill: parent
            hoverEnabled: true

            property bool dragging: false
            property int hoveredIndex: -1

            onPressed: mouse => {
                plot.forceActiveFocus();
                const hit = root.handleAt(mouse.x, mouse.y);
                if (hit < 0)
                    return;
                root.selectedIndex = hit;
                dragging = true;
                root.item.beginEdit();
            }
            onPositionChanged: mouse => {
                if (dragging)
                    root.moveSelected(mouse.x, mouse.y);
                else
                    hoveredIndex = root.handleAt(mouse.x, mouse.y);
            }
            onReleased: {
                if (!dragging)
                    return;
                dragging = false;
                root.item.commitEdit();
            }
            onExited: hoveredIndex = -1
        }

        // Handle dots. The selected one fills with the accent and the one in
        // grab range grows.
        Repeater {
            model: root.points

            Rectangle {
                required property int index

                readonly property point center: root.handleCenter(root.points[index])
                readonly property bool selected: index === root.selectedIndex
                readonly property bool hovered: index === plotMouse.hoveredIndex || (plotMouse.dragging && selected)

                x: center.x - width / 2
                y: center.y - height / 2
                width: root.handleSize
                height: root.handleSize
                radius: width / 2
                color: selected ? Theme.ramp.selectedColor : Theme.ramp.pointColor
                scale: hovered ? 1.5 : 1
                Behavior on scale {
                    NumberAnimation {
                        duration: 90
                    }
                }
            }
        }
    }
}
