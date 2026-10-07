import QtQuick
import Enzo

// A node's path with the node itself picked out.
//
// e.g.
//   NodePathLabel {
//       path: ["assets", "component", "merge1"]
//   }
Row {
    id: root

    required property var path

    spacing: 5

    Repeater {
        model: root.path

        delegate: Row {
            id: pathName

            required property int index
            required property string modelData
            readonly property bool isNode: index === root.path.length - 1

            spacing: 5

            Text {
                text: "/"
                color: Theme.nodePath.separatorColor
                font.family: Theme.var.fontMono
                font.pixelSize: 11
            }
            Text {
                text: pathName.modelData
                color: pathName.isNode ? Theme.nodePath.nodeColor : Theme.nodePath.nameColor
                font.family: Theme.var.fontMono
                font.pixelSize: 11
            }
        }
    }
}
