import QtQuick
import Enzo
import "../Style"

// One parameter, picking the control for its kind. Groups and dividers span the
// full width, every other kind sits beside its label.
Item {
    id: row

    // Layout constants.
    readonly property real parameterHeight: Constants.parameterHeight
    readonly property real labelGap: 25
    readonly property real labelFontSize: 12

    required property var item

    // Column width for the label, assigned by the list.
    property real labelColumnWidth: 0

    // Horizontal padding around the row's content, assigned by the list.
    property real contentInset: 0

    // Kinds that run edge to edge instead of sitting inside the inset.
    readonly property var fullWidthKinds: ["divider"]

    readonly property real bodyInset: (item && fullWidthKinds.includes(item.kind)) ? 0 : contentInset

    // Kinds the row adds no side label for, either because the control draws its
    // own or because it has none.
    readonly property var unlabeledKinds: ["group", "ramp", "divider"]

    // Whether this row shows a label beside its control.
    readonly property bool hasLabel: item && !unlabeledKinds.includes(item.kind) && !item.hidden && !item.labelHidden

    // Width this row wants for its label, 0 when it shows none. A group reports
    // its nested rows so the column can span them.
    readonly property real implicitLabelWidth: {
        if (item && item.kind === "group")
            return groupLoader.item ? groupLoader.item.implicitLabelWidth : 0;
        return hasLabel ? Math.ceil(labelMetrics.advanceWidth) : 0;
    }

    width: parent ? parent.width : 0
    visible: item && !item.hidden
    height: visible ? leafBody.implicitHeight + groupLoader.implicitHeight : 0
    opacity: (item && item.enabled) ? 1 : 0.4

    TextMetrics {
        id: labelMetrics
        text: row.hasLabel ? row.item.label : ""
        font.family: Theme.var.fontSans
        font.pixelSize: row.labelFontSize
    }

    Loader {
        id: leafBody
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: row.bodyInset
        anchors.rightMargin: row.bodyInset
        sourceComponent: {
            if (!row.item || row.item.kind === "group")
                return null;
            if (row.item.kind === "divider")
                return dividerComp;
            return parameterComp;
        }
    }

    // Loaded by URL to break the ParameterRow/Group type cycle QML errors on.
    Loader {
        id: groupLoader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: row.bodyInset
        anchors.rightMargin: row.bodyInset
        Component.onCompleted: {
            if (row.item && row.item.kind === "group")
                setSource("Group.qml", {
                    "item": row.item
                });
        }
        onLoaded: item.labelColumnWidth = Qt.binding(() => row.labelColumnWidth)
    }

    Component {
        id: parameterComp

        Row {
            width: parent.width
            spacing: row.hasLabel ? row.labelGap : 0

            Text {
                visible: row.hasLabel
                width: row.hasLabel ? row.labelColumnWidth : 0
                height: row.parameterHeight
                verticalAlignment: Text.AlignVCenter
                text: row.item.label
                color: Theme.var.text
                font.family: Theme.var.fontSans
                font.pixelSize: row.labelFontSize
                elide: Text.ElideRight
            }

            Loader {
                width: parent.width - (row.hasLabel ? row.labelColumnWidth + row.labelGap : 0)
                sourceComponent: {
                    switch (row.item.kind) {
                    case "float":
                        return floatComp;
                    case "int":
                        return intComp;
                    case "bool":
                    case "toggle":
                        return toggleComp;
                    case "xyz":
                        return vecComp;
                    case "string":
                        return stringComp;
                    case "dropdown":
                        return dropComp;
                    case "ramp":
                        return rampComp;
                    }
                    return null;
                }
            }
        }
    }

    Component {
        id: dividerComp
        Divider {
            item: row.item
        }
    }
    Component {
        id: floatComp
        FloatSlider {
            item: row.item
        }
    }
    Component {
        id: intComp
        IntSlider {
            item: row.item
        }
    }
    Component {
        id: toggleComp
        Toggle {
            item: row.item
        }
    }
    Component {
        id: vecComp
        VectorParameter {
            item: row.item
        }
    }
    Component {
        id: stringComp
        StringParameter {
            item: row.item
        }
    }
    Component {
        id: dropComp
        DropdownParameter {
            item: row.item
        }
    }
    Component {
        id: rampComp
        RampEditor {
            item: row.item
        }
    }
}
