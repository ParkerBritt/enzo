import QtQuick
import "../Components"

// One slider per component of a vector parameter.
Row {
    id: vec

    required property var item
    readonly property int count: item.vectorSize
    spacing: 4

    Repeater {
        model: vec.count

        delegate: Slider {
            required property int index

            width: (vec.width - vec.spacing * (vec.count - 1)) / vec.count
            implicitHeight: 22
            from: vec.item.minimum
            to: vec.item.maximum
            value: vec.item.valueAt(index)
            onMoved: (v) => vec.item.setValueAt(index, v)

            // valueAt is a call, so an external edit is mirrored in by hand.
            Connections {
                target: vec.item
                function onValueChanged() { value = vec.item.valueAt(index) }
            }
        }
    }
}
