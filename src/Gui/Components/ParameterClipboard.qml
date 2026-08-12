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

    function copy(sourceNodeName, name, label, componentIndex) {
        nodeName = sourceNodeName;
        paramName = name;
        paramLabel = label;
        index = componentIndex;
    }
}
