import QtQuick

// One parameter, picking the control for its kind. Groups and spacers span the
// full width, every other kind sits beside its label.
Item {
    id: row

    required property var item

    // Column width for the label, assigned by the list.
    property real labelColumnWidth: 0

    // Width this row wants for its label, 0 when it shows none. A group reports
    // its nested rows so the column can span them.
    readonly property bool hasLabel: item && item.kind !== "group" && item.kind !== "spacer" && !item.hidden && !item.labelHidden
    readonly property real implicitLabelWidth: {
        if (item && item.kind === "group")
            return groupLoader.item ? groupLoader.item.implicitLabelWidth : 0
        return hasLabel ? Math.ceil(labelMetrics.advanceWidth) : 0
    }

    readonly property real controlHeight: 22
    readonly property real columnSpacing: 8

    width: parent ? parent.width : 0
    visible: item && !item.hidden
    height: visible ? leafBody.implicitHeight + groupLoader.implicitHeight : 0
    opacity: (item && item.enabled) ? 1 : 0.4

    TextMetrics {
        id: labelMetrics
        text: row.hasLabel ? row.item.label : ""
        font.family: Theme.fontUi
        font.pixelSize: 12
    }

    Loader {
        id: leafBody
        width: row.width
        sourceComponent: {
            if (!row.item || row.item.kind === "group") return null
            if (row.item.kind === "spacer") return spacerComp
            return parameterComp
        }
    }

    // Loaded by URL to break the ParameterRow/Group type cycle QML errors on.
    Loader {
        id: groupLoader
        width: row.width
        Component.onCompleted: {
            if (row.item && row.item.kind === "group")
                setSource("Group.qml", { "item": row.item })
        }
        onLoaded: item.labelColumnWidth = Qt.binding(() => row.labelColumnWidth)
    }

    Component {
        id: parameterComp

        Row {
            width: parent.width
            spacing: row.hasLabel ? row.columnSpacing : 0

            Text {
                visible: row.hasLabel
                width: row.hasLabel ? row.labelColumnWidth : 0
                height: row.controlHeight
                verticalAlignment: Text.AlignVCenter
                text: row.item.label
                color: Theme.label
                font.family: Theme.fontUi
                font.pixelSize: 12
                elide: Text.ElideRight
            }

            Loader {
                width: parent.width - (row.hasLabel ? row.labelColumnWidth + row.columnSpacing : 0)
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

    Component { id: spacerComp; Item { implicitHeight: 8 } }
    Component { id: floatComp; FloatSlider { item: row.item } }
    Component { id: intComp; IntSlider { item: row.item } }
    Component { id: toggleComp; Toggle { item: row.item } }
    Component { id: vecComp; VectorParameter { item: row.item } }
    Component { id: stringComp; StringParameter { item: row.item } }
    Component { id: dropComp; Dropdown { item: row.item } }
    Component { id: rampComp; RampEditor { item: row.item } }
}
