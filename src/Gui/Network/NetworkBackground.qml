import QtQuick
import Enzo

// The dotted grid behind the network. The shader draws the pan and zoom, the
// item itself is never transformed.
ShaderEffect {
    fragmentShader: "qrc:/NetworkDots.frag.qsb"

    property real zoom: 1
    property point pan
    property size canvas: Qt.size(width, height)
    property color dotColor: Theme.network.dotColor
}
