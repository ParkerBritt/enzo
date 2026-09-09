import QtQuick
import Enzo
import "../Components"
import "../Style"

// One slider per component of a vector parameter, each marked with an
// axis-coloured swatch.
Row {
    id: vec

    required property var item
    readonly property int count: item ? item.vectorSize : 0
    spacing: 4

    // Per component swatch colour, matching the axis tint the geometry
    // spreadsheet gives its x y z headers. A fourth component falls back to
    // muted grey.
    readonly property var axisColors: [
        Theme.var.axisX,
        Theme.var.axisY,
        Theme.var.axisZ,
        Theme.var.textMuted
    ]

    Repeater {
        model: vec.count

        delegate: Slider {
            id: componentSlider
            required property int index

            width: (vec.width - vec.spacing * (vec.count - 1)) / vec.count
            implicitHeight: Constants.parameterHeight
            axisColor: vec.axisColors[Math.min(index, 3)]
            from: vec.item ? vec.item.minimum : 0
            to: vec.item ? vec.item.maximum : 1
            clampMin: vec.item ? vec.item.minLocked : true
            clampMax: vec.item ? vec.item.maxLocked : true
            value: vec.item ? vec.item.valueAt(index) : 0
            hasExpression: vec.item ? vec.item.hasExpressionAt(index) : false
            expressionText: vec.item ? vec.item.expressionAt(index) : ""
            expressionInvalid: vec.item ? vec.item.expressionErrorAt(index).length > 0 : false
            paramLabel: vec.item ? vec.item.label : ""
            paramKind: vec.item ? vec.item.kind : ""
            paramName: vec.item ? vec.item.name : ""
            paramNodeName: vec.item ? vec.item.nodeName : ""
            componentIndex: index
            evaluator: (text) => vec.item ? vec.item.previewExpressionAt(index, text) : ({value: 0, invalid: false})
            onPressed: if (vec.item) vec.item.beginEdit()
            onMoved: (v) => { if (vec.item) vec.item.setValueAt(index, v) }
            onReleased: if (vec.item) vec.item.commitEdit()
            onExpressionEntered: (expr) => {
                if (!vec.item) return
                vec.item.beginEdit()
                vec.item.setExpressionAt(index, expr)
                vec.item.commitEdit()
            }
            onExpressionReverted: {
                if (!vec.item) return
                vec.item.beginEdit()
                vec.item.clearExpressionAt(index)
                vec.item.commitEdit()
            }

            // valueAt/expressionAt are calls, so an external edit is mirrored in by hand.
            Connections {
                target: vec.item
                function onValueChanged() {
                    componentSlider.value = vec.item.valueAt(index)
                    componentSlider.hasExpression = vec.item.hasExpressionAt(index)
                    componentSlider.expressionText = vec.item.expressionAt(index)
                    componentSlider.expressionInvalid = vec.item.expressionErrorAt(index).length > 0
                }
            }
        }
    }
}
