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
    onMoved: (v) => { if (item) item.value = v }
}
