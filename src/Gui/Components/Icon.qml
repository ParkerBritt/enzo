import QtQuick
import Enzo

// A tinted monochrome icon loaded by name from the Lucide set or Enzo's own icons.
Item {
    id: root

    property string name
    property color color: Theme.var.text
    property real size: 16

    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.centerIn: parent
        width: root.size
        height: root.size
        source: root.name === "" ? "" : "image://icon/" + root.name + "?color=" + encodeURIComponent(root.color)
        sourceSize.width: Math.round(root.size * 2)
        sourceSize.height: Math.round(root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
    }
}
