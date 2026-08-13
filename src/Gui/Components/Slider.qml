import QtQuick
import Enzo
import "../Style"

// Horizontal slider that opens for typing on a click, or scrubs relative to drag distance.
Item {
    id: root

    property real value: 0
    property real from: 0
    property real to: 1
    property bool integer: false
    property bool clampMin: true
    property bool clampMax: true
    property bool editing: false

    // A component driven by a formula instead of a literal shows as a pill in
    // place of the usual track and fill.
    property bool hasExpression: false
    property string expressionText: ""
    property bool expressionInvalid: false

    // Named in the floating editor's header, e.g. "Density" / "float".
    property string paramLabel: ""
    property string paramKind: ""

    // This component's own identity, used to build a prm()/prmI()/prmS()
    // reference to it when it is copied from the right-click menu.
    property string paramNodeName: ""
    property string paramName: ""
    property int componentIndex: 0

    // Evaluates typed text for the floating editor's live preview, without committing it.
    property var evaluator: null

    signal moved(real value)
    signal expressionEntered(string expression)
    signal expressionReverted

    signal pressed
    signal released

    implicitHeight: Constants.parameterHeight

    readonly property real fraction: to > from ? Math.max(0, Math.min(1, (value - from) / (to - from))) : 0

    readonly property string displayValue: root.integer ? Math.round(root.value).toString() : root.value.toFixed(3)

    // The pill only shows at rest, giving way to the plain text field once a click opens it for editing.
    readonly property bool showingExpression: root.hasExpression && !root.editing

    // Typing (whether into a plain value or an existing formula) always looks
    // like an expression box rather than a slider, since any text can turn
    // into an expression on commit.
    readonly property bool expressionStyled: root.showingExpression || root.editing

    // Tracks the unrounded value through a drag so sub-integer pixel deltas
    // still accumulate instead of getting rounded away on every step.
    property real dragValue: 0

    function beginDrag() {
        dragValue = value;
    }

    function applyDelta(pixelDelta) {
        dragValue += (pixelDelta / width) * (to - from);
        if (clampMin)
            dragValue = Math.max(from, dragValue);
        if (clampMax)
            dragValue = Math.min(to, dragValue);
        root.moved(integer ? Math.round(dragValue) : dragValue);
    }

    function beginEdit() {
        editField.text = root.hasExpression ? root.expressionText : root.displayValue;
        root.editing = true;
        editField.selectAll();
        editField.forceActiveFocus();
    }

    // Clamps a number to [from, to] as configured and moves to it, the shared
    // tail of committing a typed value or pasting one.
    function applyNumber(parsed) {
        let v = parsed;
        if (clampMin)
            v = Math.max(from, v);
        if (clampMax)
            v = Math.min(to, v);
        root.pressed();
        root.moved(integer ? Math.round(v) : v);
        root.released();
    }

    // A typed number commits as a value, any other text commits as an expression.
    function commitText(text) {
        text = text.trim();
        if (text.length === 0)
            return;
        const parsed = Number(text);
        if (isNaN(parsed)) {
            root.expressionEntered(text);
            return;
        }
        root.applyNumber(parsed);
    }

    // Casts a pasted value to a number, same as the engine casts a stored
    // literal between parameter types, falling back to 0 when it doesn't
    // parse as one (e.g. a string copied from a dropdown or text field).
    function pasteValue(pasted) {
        const parsed = Number(pasted);
        root.applyNumber(isNaN(parsed) ? 0 : parsed);
    }

    function commitEdit() {
        if (!root.editing)
            return;
        root.editing = false;
        root.commitText(editField.text);
        editField.focus = false;
    }

    function cancelEdit() {
        if (!root.editing)
            return;
        root.editing = false;
        editField.focus = false;
    }

    // Returns whether a point in root's coordinates falls within an item also laid out in root's space.
    function within(item, px, py) {
        return px >= item.x && px <= item.x + item.width && py >= item.y && py <= item.y + item.height;
    }

    function openEditor() {
        expressionEditor.openFor(root.expressionText);
    }

    Rectangle {
        id: track
        anchors.fill: parent
        radius: Theme.parameter.borderRadius
        color: root.expressionStyled ? (root.expressionInvalid ? Theme.expression.invalidBackgroundColor : Theme.expression.backgroundColor) : Theme.parameter.backgroundColor
        border.color: root.expressionStyled ? (root.expressionInvalid ? Theme.expression.invalidBorderColor : Theme.expression.borderColor) : Theme.parameter.lineColor

        // The accent fill floats inside the frame with a small inset on every
        // side so the rounded track border stays visible around it.
        Rectangle {
            id: fill
            visible: !root.expressionStyled
            readonly property real inset: 3
            x: fill.inset
            y: fill.inset
            height: parent.height - fill.inset * 2
            width: (parent.width - fill.inset * 2) * root.fraction
            radius: Theme.parameter.borderRadius - fill.inset
            color: Theme.slider.fillColor
        }

        Text {
            visible: !root.editing && !root.hasExpression
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.displayValue
            color: Theme.var.text
            font.family: Theme.var.fontMono
            font.pixelSize: 12
        }

        TextInput {
            id: editField
            visible: root.editing
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            verticalAlignment: TextInput.AlignVCenter
            horizontalAlignment: TextInput.AlignHCenter
            clip: true
            selectByMouse: true
            color: Theme.var.text
            font.family: Theme.var.fontMono
            font.pixelSize: 12
            onAccepted: root.commitEdit()
            onEditingFinished: root.commitEdit()
            Keys.onEscapePressed: root.cancelEdit()
        }

        Rectangle {
            id: fxBadge
            visible: root.showingExpression
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            radius: 4
            color: Theme.expression.badgeBackgroundColor
            width: fxLabel.implicitWidth + 10
            height: fxLabel.implicitHeight + 4

            Text {
                id: fxLabel
                anchors.centerIn: parent
                text: "fx"
                font.italic: true
                font.bold: true
                font.family: Theme.var.fontMono
                font.pixelSize: 9
                color: Theme.expression.badgeColor
            }
        }

        Rectangle {
            id: resultChip
            visible: root.showingExpression
            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            radius: 5
            color: Theme.parameter.backgroundColor
            height: 20
            width: resultRow.implicitWidth + 14

            Row {
                id: resultRow
                anchors.centerIn: parent
                spacing: 4

                Text {
                    text: root.expressionInvalid ? "⚠" : "="
                    color: root.expressionInvalid ? Theme.expression.invalidResultColor : Theme.expression.resultColor
                    font.pixelSize: 11
                }
                Text {
                    text: root.displayValue
                    color: root.expressionInvalid ? Theme.expression.invalidResultColor : Theme.expression.resultColor
                    font.family: Theme.var.fontMono
                    font.pixelSize: 11
                }
            }
        }

        Text {
            visible: root.showingExpression
            anchors.left: fxBadge.right
            anchors.leftMargin: 8
            anchors.right: resultChip.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            text: root.expressionText
            color: Theme.expression.codeColor
            font.family: Theme.var.fontMono
            font.pixelSize: 11
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.editing
        cursorShape: root.hasExpression ? Qt.PointingHandCursor : Qt.SizeHorCursor
        property point pressPos
        property real lastX
        property bool dragging: false

        onPressed: mouse => {
            pressPos = Qt.point(mouse.x, mouse.y);
            lastX = mouse.x;
            dragging = false;
        }
        onPositionChanged: mouse => {
            // A formula is edited by clicking it, not scrubbed like a plain value.
            if (root.hasExpression)
                return;
            if (!dragging) {
                if (Math.abs(mouse.x - pressPos.x) < 3)
                    return;
                dragging = true;
                root.beginDrag();
                root.pressed();
            }
            root.applyDelta(mouse.x - lastX);
            lastX = mouse.x;
        }
        onReleased: mouse => {
            if (dragging) {
                dragging = false;
                root.released();
            } else if (root.hasExpression && (root.within(fxBadge, mouse.x, mouse.y) || root.within(resultChip, mouse.x, mouse.y))) {
                root.openEditor();
            } else {
                root.beginEdit();
            }
        }
    }

    ExpressionEditor {
        id: expressionEditor
        y: root.height + 6
        paramLabel: root.paramLabel
        paramKind: root.paramKind
        evaluator: root.evaluator
        liveValue: root.value
        onCommitted: text => root.commitText(text)
        onReverted: root.expressionReverted()
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: mouse => contextMenu.popup(mouse.x, mouse.y)
    }

    ParameterContextMenu {
        id: contextMenu
        nodeName: root.paramNodeName
        paramName: root.paramName
        paramLabel: root.paramLabel
        componentIndex: root.componentIndex
        kind: root.paramKind
        hasExpression: root.hasExpression
        value: root.value
        expression: root.expressionText
        onEditRequested: root.openEditor()
        onRevertRequested: root.expressionReverted()
        onPasteRequested: expr => root.expressionEntered(expr)
        onPasteValueRequested: v => root.pasteValue(v)
    }
}
