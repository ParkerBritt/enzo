import QtQuick
import QtQuick.Shapes
import Enzo
import "../Components"

// Shows a pair of angles as a dial, with a field for each bound. Zero degrees
// is on the right and angles run counter clockwise.
Item {
    id: dial

    required property var item

    readonly property real dialSize: 122
    readonly property real centre: dialSize / 2
    readonly property real ringRadius: dialSize * 44 / 120
    readonly property real ringWidth: dialSize * 16 / 120

    readonly property real handleReach: 11

    readonly property real from: item ? item.minimum : 0
    readonly property real to: item ? item.maximum : 360
    readonly property real span: to - from

    property real start: item ? item.valueAt(0) : 0
    property real end: item ? item.valueAt(1) : 0

    // Keeps the low bound under the high one when the ring is dragged. Typed
    // values and formulas can still set them either way round.
    readonly property bool ordered: item ? item.styleOptions.ordered === true : false

    readonly property string caption: item ? (item.styleOptions.caption || "") : ""

    property bool expressionDriven: item ? (item.hasExpressionAt(0) || item.hasExpressionAt(1)) : false

    readonly property real sweep: dial.end - dial.start

    // Flips the angles, since the screen measures them clockwise.
    readonly property real screenStart: -dial.start
    readonly property real screenSweep: -dial.sweep

    readonly property real gap: 8

    readonly property real minFieldWidth: 140
    readonly property bool fieldsBeside: width >= dialSize + gap + minFieldWidth

    implicitHeight: fieldsBeside ? Math.max(dialSize, fields.height) : dialSize + gap + fields.height

    function angleAt(px, py) {
        return dial.intoRange(-Math.atan2(py - dial.centre, px - dial.centre) * 180 / Math.PI);
    }

    // Rotates from one angle to another, keeps the delta when crossing over 0.
    function turnBetween(fromAngle, toAngle) {
        return ((toAngle - fromAngle + 540) % 360) - 180;
    }

    // Brings an angle into range. Wraps when the range is a full turn, clamps
    // when it is less.
    function intoRange(angle) {
        if (dial.span >= 360)
            return dial.from + (((angle - dial.from) % dial.span) + dial.span) % dial.span;
        return Math.max(dial.from, Math.min(dial.to, angle));
    }

    // Writes both angles at once. When they run past the range both shift by 360,
    // never just one, so the sweep between them stays the same.
    function setBounds(startAngle, endAngle) {
        if (!dial.item)
            return;
        const low = dial.ordered ? Math.min(startAngle, endAngle) : startAngle;
        const high = dial.ordered ? Math.max(startAngle, endAngle) : endAngle;
        const turns = dial.span >= 360 ? low - dial.intoRange(low) : 0;
        dial.item.setValueAt(0, low - turns);
        dial.item.setValueAt(1, high - turns);
    }

    function handlePoint(angle) {
        const radians = angle * Math.PI / 180;
        return Qt.point(dial.centre + Math.cos(radians) * dial.ringRadius, dial.centre - Math.sin(radians) * dial.ringRadius);
    }

    function handleAt(px, py) {
        for (let index = 0; index < 2; ++index) {
            const point = dial.handlePoint(index === 0 ? dial.start : dial.end);
            if (Math.hypot(px - point.x, py - point.y) <= dial.handleReach)
                return index;
        }
        return -1;
    }

    function onBand(px, py) {
        const distance = Math.hypot(px - dial.centre, py - dial.centre);
        if (Math.abs(distance - dial.ringRadius) > dial.ringWidth / 2)
            return false;
        const along = dial.sweep >= 0 ? dial.angleAt(px, py) - dial.start : dial.start - dial.angleAt(px, py);
        return ((along % 360) + 360) % 360 <= Math.abs(dial.sweep);
    }

    // valueAt is a call, not a property, so an edit from elsewhere is copied in
    // by hand.
    Connections {
        target: dial.item
        function onValueChanged() {
            dial.start = dial.item.valueAt(0);
            dial.end = dial.item.valueAt(1);
            dial.expressionDriven = dial.item.hasExpressionAt(0) || dial.item.hasExpressionAt(1);
        }
    }

    component Ring: ShapePath {
        id: ringPath

        property real radius: dial.ringRadius
        property real startAngle: 0
        property real sweepAngle: 360

        fillColor: "transparent"
        capStyle: ShapePath.FlatCap
        PathAngleArc {
            centerX: dial.centre
            centerY: dial.centre
            radiusX: ringPath.radius
            radiusY: ringPath.radius
            startAngle: ringPath.startAngle
            sweepAngle: ringPath.sweepAngle
        }
    }

    Item {
        id: ring
        width: dial.dialSize
        height: dial.dialSize
        x: dial.fieldsBeside ? 0 : (dial.width - dial.dialSize) / 2
        opacity: dial.expressionDriven ? 0.4 : 1

        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            Ring {
                strokeColor: Theme.rangeDial.trackColor
                strokeWidth: dial.ringWidth
            }

            Ring {
                radius: dial.ringRadius + dial.ringWidth / 2
                strokeColor: Theme.rangeDial.edgeColor
                strokeWidth: 1
            }

            Ring {
                radius: dial.ringRadius - dial.ringWidth / 2
                strokeColor: Theme.rangeDial.edgeColor
                strokeWidth: 1
            }

            Ring {
                startAngle: dial.screenStart
                sweepAngle: dial.screenSweep
                strokeColor: Theme.rangeDial.arcColor
                strokeWidth: dial.ringWidth - 3
            }
        }

        // The mark for zero degrees.
        Rectangle {
            x: dial.centre + dial.ringRadius + dial.ringWidth / 2 + 3
            y: dial.centre - 0.5
            width: 5
            height: 1
            color: Theme.rangeDial.tickColor
        }

        Column {
            anchors.centerIn: parent
            spacing: 1

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Math.round(dial.sweep) + "°"
                color: Theme.var.textStrong
                font.family: Theme.var.fontMono
                font.pixelSize: 19
            }

            Text {
                visible: dial.caption.length > 0
                anchors.horizontalCenter: parent.horizontalCenter
                text: dial.caption.toUpperCase()
                color: Theme.var.textMuted
                font.family: Theme.var.fontSans
                font.pixelSize: 7
                font.letterSpacing: 1
            }
        }

        Repeater {
            model: 2

            delegate: Rectangle {
                id: handle
                required property int index

                readonly property real angle: handle.index === 0 ? dial.start : dial.end
                readonly property point centre: dial.handlePoint(handle.angle)
                readonly property bool active: ringArea.hoveredHandle === handle.index

                x: handle.centre.x - width / 2
                y: handle.centre.y - height / 2
                width: handle.active ? 20 : 18
                height: handle.active ? 9 : 7
                radius: 3.5
                rotation: -handle.angle
                color: handle.active ? Theme.rangeDial.handleHoverColor : Theme.rangeDial.handleColor
            }
        }

        MouseArea {
            id: ringArea
            anchors.fill: parent
            enabled: !dial.expressionDriven
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton

            property bool turning: false

            // The handle being dragged, or -1 when the arc itself is.
            property int grabbedHandle: -1

            // The angles the drag is turning. Kept here so a swap does not feed
            // back into the next step.
            property real turnedStart: 0
            property real turnedEnd: 0
            property real lastCursor: 0

            readonly property int hoveredHandle: turning ? grabbedHandle : (containsMouse ? dial.handleAt(mouseX, mouseY) : -1)

            cursorShape: {
                if (hoveredHandle >= 0)
                    return Qt.SizeHorCursor;
                return turning ? Qt.ClosedHandCursor : Qt.ArrowCursor;
            }

            onPressed: mouse => {
                grabbedHandle = dial.handleAt(mouse.x, mouse.y);
                if (grabbedHandle < 0 && !dial.onBand(mouse.x, mouse.y)) {
                    mouse.accepted = false;
                    return;
                }
                turning = true;
                turnedStart = dial.start;
                turnedEnd = dial.end;
                lastCursor = dial.angleAt(mouse.x, mouse.y);
                dial.item.beginEdit();
            }
            onPositionChanged: mouse => {
                if (!turning)
                    return;
                const cursor = dial.angleAt(mouse.x, mouse.y);
                const turn = dial.turnBetween(lastCursor, cursor);
                lastCursor = cursor;
                if (grabbedHandle !== 1)
                    turnedStart += turn;
                if (grabbedHandle !== 0)
                    turnedEnd += turn;
                dial.setBounds(turnedStart, turnedEnd);
            }
            onReleased: {
                if (!turning)
                    return;
                turning = false;
                grabbedHandle = -1;
                dial.item.commitEdit();
            }
        }
    }

    component BoundField: Row {
        id: field

        required property string boundLabel
        required property int boundIndex

        spacing: 6

        Text {
            width: 30
            height: bound.height
            verticalAlignment: Text.AlignVCenter
            text: field.boundLabel
            color: Theme.var.textMuted
            font.family: Theme.var.fontSans
            font.pixelSize: 10
        }

        Slider {
            id: bound

            width: field.width - 36
            implicitHeight: 22
            from: dial.from
            to: dial.to
            clampMin: dial.item ? dial.item.minLocked : true
            clampMax: dial.item ? dial.item.maxLocked : true
            value: dial.item ? dial.item.valueAt(field.boundIndex) : 0
            hasExpression: dial.item ? dial.item.hasExpressionAt(field.boundIndex) : false
            expressionText: dial.item ? dial.item.expressionAt(field.boundIndex) : ""
            expressionInvalid: dial.item ? dial.item.expressionErrorAt(field.boundIndex).length > 0 : false
            paramLabel: dial.item ? dial.item.label : ""
            paramKind: dial.item ? dial.item.kind : ""
            paramName: dial.item ? dial.item.name : ""
            paramNodeName: dial.item ? dial.item.nodeName : ""
            componentIndex: field.boundIndex
            evaluator: text => dial.item ? dial.item.previewExpressionAt(field.boundIndex, text) : ({
                        value: 0,
                        invalid: false
                    })
            onPressed: if (dial.item)
                dial.item.beginEdit()
            onMoved: v => {
                if (dial.item)
                    dial.item.setValueAt(field.boundIndex, v);
            }
            onReleased: if (dial.item)
                dial.item.commitEdit()
            onExpressionEntered: expr => {
                if (!dial.item)
                    return;
                dial.item.beginEdit();
                dial.item.setExpressionAt(field.boundIndex, expr);
                dial.item.commitEdit();
            }
            onExpressionReverted: {
                if (!dial.item)
                    return;
                dial.item.beginEdit();
                dial.item.clearExpressionAt(field.boundIndex);
                dial.item.commitEdit();
            }

            Connections {
                target: dial.item
                function onValueChanged() {
                    bound.value = dial.item.valueAt(field.boundIndex);
                    bound.hasExpression = dial.item.hasExpressionAt(field.boundIndex);
                    bound.expressionText = dial.item.expressionAt(field.boundIndex);
                    bound.expressionInvalid = dial.item.expressionErrorAt(field.boundIndex).length > 0;
                }
            }
        }
    }

    Column {
        id: fields
        spacing: 6
        x: dial.fieldsBeside ? dial.dialSize + dial.gap : 0
        y: dial.fieldsBeside ? (dial.dialSize - height) / 2 : dial.dialSize + dial.gap
        width: dial.fieldsBeside ? dial.width - dial.dialSize - dial.gap : dial.width

        BoundField {
            width: fields.width
            boundLabel: "Start"
            boundIndex: 0
        }

        BoundField {
            width: fields.width
            boundLabel: "End"
            boundIndex: 1
        }
    }
}
