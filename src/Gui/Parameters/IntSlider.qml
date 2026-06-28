import "../Components"

Slider {
    required property var item

    implicitHeight: 22
    integer: true
    from: item.minimum
    to: item.maximum
    value: item.value
    onMoved: (v) => item.value = v
}
