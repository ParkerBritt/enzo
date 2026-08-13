pragma Singleton
import QtQuick

// Holds the most recently copied parameter component, referenced with
// prm()/prmI()/prmS() when pasted onto another field.
//
// e.g. copying scatter1's Density then pasting onto Seed sets Seed's
// expression to prm("scatter1.amp")
QtObject {
    readonly property bool hasValue: paramName.length > 0

    property string nodeName: ""
    property string paramName: ""
    property string paramLabel: ""
    property int index: 0
    property string kind: ""
    property var value: undefined
    // The component's raw expression source, empty when it holds a literal.
    property string expression: ""

    function copy(sourceNodeName, name, label, componentIndex, componentKind, componentValue, componentExpression) {
        nodeName = sourceNodeName;
        paramName = name;
        paramLabel = label;
        index = componentIndex;
        kind = componentKind;
        value = componentValue;
        expression = componentExpression;
    }
}
