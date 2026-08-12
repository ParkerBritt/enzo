import Enzo
import "../Components"
import "../Style"

Slider {
    required property var item

    implicitHeight: Constants.parameterHeight
    integer: true
    from: item ? item.minimum : 0
    to: item ? item.maximum : 1
    clampMin: item ? item.minLocked : true
    clampMax: item ? item.maxLocked : true
    value: item ? item.value : 0
    hasExpression: item ? item.hasExpression : false
    expressionText: item ? item.expression : ""
    expressionInvalid: item ? item.expressionError.length > 0 : false
    paramLabel: item ? item.label : ""
    paramKind: item ? item.kind : ""
    paramName: item ? item.name : ""
    paramNodeName: item ? item.nodeName : ""
    evaluator: (text) => item ? item.previewExpressionAt(0, text) : ({value: 0, invalid: false})
    onPressed: if (item) item.beginEdit()
    onMoved: (v) => { if (item) item.value = v }
    onReleased: if (item) item.commitEdit()
    onExpressionEntered: (expr) => {
        if (!item) return
        item.beginEdit()
        item.setExpressionAt(0, expr)
        item.commitEdit()
    }
    onExpressionReverted: {
        if (!item) return
        item.beginEdit()
        item.clearExpressionAt(0)
        item.commitEdit()
    }
}
