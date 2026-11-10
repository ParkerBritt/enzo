import QtQuick
import QtQuick.Controls

// Window whose contents keep the same physical size on every screen.
Window {
    id: root

    // Pixel density in px/mm that the UI is laid out for, about a 27" 1440p screen.
    readonly property real referencePixelDensity: 4.3

    // Limits the pixel scale. The minimum keeps small text readable on low density screens.
    readonly property real minimumPixelScale: 0.9
    readonly property real maximumPixelScale: 3

    // Screen pixels per layout unit for the screen this window is on, 1 when the screen reports no size.
    readonly property real pixelScale: {
        const measured = Screen.pixelDensity / referencePixelDensity;
        if (!(measured > 0) || !isFinite(measured))
            return 1;
        return Math.min(Math.max(measured, minimumPixelScale), maximumPixelScale);
    }

    // Scale for the screen this window is on, times the user's scale.
    readonly property real uiScale: pixelScale / Screen.devicePixelRatio * interfaceSettings.scale

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
