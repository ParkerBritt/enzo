import QtQuick
import QtQuick.Controls

// Window whose contents keep the same physical size on every screen.
Window {
    id: root

    // Pixel density in px/mm that the UI is laid out for, about a 27" 1440p screen.
    readonly property real referencePixelDensity: 4.3

    // Scale for the screen this window is on.
    readonly property real uiScale: Screen.pixelDensity / referencePixelDensity / Screen.devicePixelRatio

    default property alias content: scaledRoot.data

    Item {
        id: scaledRoot

        width: parent.width / root.uiScale
        height: parent.height / root.uiScale
        scale: root.uiScale
        transformOrigin: Item.TopLeft
    }

    // Scales the popup and menu layer with the contents so popups centre and stay inside the window.
    Binding {
        target: scaledRoot.Overlay.overlay
        property: "width"
        value: scaledRoot.width
    }
    Binding {
        target: scaledRoot.Overlay.overlay
        property: "height"
        value: scaledRoot.height
    }
    Binding {
        target: scaledRoot.Overlay.overlay
        property: "scale"
        value: root.uiScale
    }
    Binding {
        target: scaledRoot.Overlay.overlay
        property: "transformOrigin"
        value: Item.TopLeft
    }
}
