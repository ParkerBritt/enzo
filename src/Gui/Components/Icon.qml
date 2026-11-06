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
        id: image

        anchors.centerIn: parent
        width: root.size
        height: root.size
        sourceSize.width: Math.round(root.size * 2)
        sourceSize.height: Math.round(root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
    }

    // Lucide strokes are `currentColor`, so the colour is baked into the markup.
    function rgbHex(c) {
        const part = v => ("0" + Math.round(v * 255).toString(16)).slice(-2);
        return "#" + part(c.r) + part(c.g) + part(c.b);
    }

    function reload() {
        if (root.name === "")
            return;
        const url = Theme.getIconUrl(root.name);
        if (url === "") {
            console.warn("Icon: no icon named", root.name);
            return;
        }
        const hex = rgbHex(root.color);
        const request = new XMLHttpRequest();
        request.open("GET", url);
        request.onreadystatechange = function () {
            if (request.readyState !== XMLHttpRequest.DONE)
                return;
            // The icon can be destroyed (e.g. its popup closes) while this
            // request is still in flight, which nulls out the id it closed over.
            if (!image)
                return;
            if (!request.responseText) {
                console.warn("Icon: failed to load", url);
                return;
            }
            const svg = request.responseText.replace(/currentColor/g, hex);
            image.source = "data:image/svg+xml;utf8," + encodeURIComponent(svg);
        };
        request.send();
    }

    onNameChanged: reload()
    onColorChanged: reload()
    Component.onCompleted: reload()
}
