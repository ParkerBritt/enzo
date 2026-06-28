import QtQuick

// One parameter, picking the control that matches its kind. A group and a
// spacer span the full width, every other kind sits beside a label.
Item {
    id: row

    required property var item

    // Layout constants.
    readonly property real labelWidth: 62
    readonly property real controlHeight: 22
    readonly property real columnSpacing: 8

    width: parent ? parent.width : 0
    visible: item && !item.hidden
    height: visible ? body.implicitHeight : 0
    opacity: (item && item.enabled) ? 1 : 0.4

    Loader {
        id: body
        width: row.width
        sourceComponent: {
            if (!row.item) return null
            if (row.item.kind === "group") return groupComp
            if (row.item.kind === "spacer") return spacerComp
            return parameterComp
        }
    }

    Component {
        id: parameterComp

        Row {
            width: parent.width
            spacing: row.columnSpacing

            Text {
                width: row.labelWidth
                height: row.controlHeight
                verticalAlignment: Text.AlignVCenter
                text: row.item.label
                color: Theme.label
                font.family: Theme.fontUi
                font.pixelSize: 12
                elide: Text.ElideRight
            }

            Loader {
                width: parent.width - row.labelWidth - row.columnSpacing
                sourceComponent: {
                    switch (row.item.kind) {
                    case "float": return floatComp
                    case "int": return intComp
                    case "bool":
                    case "toggle": return toggleComp
                    case "xyz": return vecComp
                    case "string": return stringComp
                    case "dropdown": return dropComp
                    case "ramp": return rampComp
                    }
                    return spacerComp
                }
            }
        }
    }

    // Group is loaded by URL rather than as a type, breaking the static cycle
    // with ParameterRow that a group's nested rows would otherwise form.
    Component {
        id: groupComp
        Loader {
            width: row.width
            Component.onCompleted: setSource("Group.qml", { "item": row.item })
        }
    }
    Component { id: spacerComp; Item { implicitHeight: 8 } }
    Component { id: floatComp; FloatSlider { item: row.item } }
    Component { id: intComp; IntSlider { item: row.item } }
    Component { id: toggleComp; Toggle { item: row.item } }
    Component { id: vecComp; VectorParameter { item: row.item } }
    Component { id: stringComp; StringParameter { item: row.item } }
    Component { id: dropComp; Dropdown { item: row.item } }
    Component { id: rampComp; RampEditor { item: row.item } }
}
