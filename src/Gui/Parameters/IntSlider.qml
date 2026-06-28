import "../Components"

Slider {
    required property var item

    implicitHeight: 22
    integer: true
    from: item ? item.minimum : 0
    to: item ? item.maximum : 1
    clampMin: item ? item.minLocked : true
    clampMax: item ? item.maxLocked : true
    value: item ? item.value : 0
    onMoved: (v) => { if (item) item.value = v }
}
