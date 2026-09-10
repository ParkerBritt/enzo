import QtQuick

// A vertical list of parameters whose labels share one column.
Column {
    id: list

    property alias model: rows.model

    // Width handed to every row, defaulting to the widest label here.
    property real labelColumnWidth: implicitLabelWidth

    // Widest label, reported up so a parent can share a column across lists.
    readonly property real implicitLabelWidth: widest
    property real widest: 0

    // Horizontal padding each row carries around its content.
    property real contentInset: 0
    spacing: 6

    function remeasure() {
        let measured = 0
        for (let index = 0; index < rows.count; index++) {
            let row = rows.itemAt(index)
            if (row) measured = Math.max(measured, row.implicitLabelWidth)
        }
        widest = measured
    }

    Repeater {
        id: rows
        onCountChanged: list.remeasure()
        delegate: ParameterRow {
            required property var modelData
            required property int index
            item: modelData
            rowIndex: index
            width: list.width
            contentInset: list.contentInset
            labelColumnWidth: list.labelColumnWidth
            onImplicitLabelWidthChanged: list.remeasure()
            Component.onCompleted: list.remeasure()
        }
    }
}
