import Enzo
import "../Components"
import "../Style"

Slider {
    required property var item

    implicitHeight: Constants.parameterHeight
    from: item ? item.minimum : 0
    to: item ? item.maximum : 1
    clampMin: item ? item.minLocked : true
    clampMax: item ? item.maxLocked : true
    value: item ? item.value : 0
    onPressed: if (item) item.beginEdit()
    onMoved: (v) => { if (item) item.value = v }
    onReleased: if (item) item.commitEdit()
    onExpressionEntered: (expr) => {
        if (!item) return
        item.beginEdit()
        item.setExpressionAt(0, expr)
        item.commitEdit()
    }
}
