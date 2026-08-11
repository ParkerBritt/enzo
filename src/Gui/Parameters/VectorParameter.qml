import QtQuick
import Enzo
import "../Components"
import "../Style"

// One slider per component of a vector parameter.
Row {
    id: vec

    required property var item
    readonly property int count: item ? item.vectorSize : 0
    spacing: 4

    Repeater {
        model: vec.count

        delegate: Slider {
            required property int index

            width: (vec.width - vec.spacing * (vec.count - 1)) / vec.count
            implicitHeight: Constants.parameterHeight
            from: vec.item ? vec.item.minimum : 0
            to: vec.item ? vec.item.maximum : 1
            clampMin: vec.item ? vec.item.minLocked : true
            clampMax: vec.item ? vec.item.maxLocked : true
            value: vec.item ? vec.item.valueAt(index) : 0
            onPressed: if (vec.item) vec.item.beginEdit()
            onMoved: (v) => { if (vec.item) vec.item.setValueAt(index, v) }
            onReleased: if (vec.item) vec.item.commitEdit()
            onExpressionEntered: (expr) => {
                if (!vec.item) return
                vec.item.beginEdit()
                vec.item.setExpressionAt(index, expr)
                vec.item.commitEdit()
            }

            // valueAt is a call, so an external edit is mirrored in by hand.
            Connections {
                target: vec.item
                function onValueChanged() { value = vec.item.valueAt(index) }
            }
        }
    }
}
