import QtQuick

// A tinted monochrome icon loaded by name from the bundled Lucide set. The SVG
// text is recoloured to `color` and handed to the image as a data url, so the
// glyph paints in the requested colour with no effect pipeline.
Item {
    id: root

    property string name
    property color color: Theme.text
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
        const hex = rgbHex(root.color);
        const request = new XMLHttpRequest();
        request.open("GET", Theme.iconsDir + root.name + ".svg");
        request.onreadystatechange = function () {
            if (request.readyState !== XMLHttpRequest.DONE)
                return;
            const svg = request.responseText.replace(/currentColor/g, hex);
            image.source = "data:image/svg+xml;utf8," + encodeURIComponent(svg);
        };
        request.send();
    }

    onNameChanged: reload()
    onColorChanged: reload()
    Component.onCompleted: reload()
}
