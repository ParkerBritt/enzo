#pragma once

/// @file
/// Central style tokens for the GUI, covering colors, typography, and metrics.
///
/// The file works in two tiers. A small set of shared tokens at the top holds
/// the values that recur all over the interface, and each component then keeps
/// everything about its own look together in one namespace.
///
/// @par Shared tokens
/// `color` holds the neutral palette and the accents, and `font` holds the
/// application wide typography defaults. Components point at these so adjusting
/// one shared tone repaints everywhere that leans on it.
///
/// @par Per component
/// Each widget owns a namespace such as `ramp` or `slider` that gathers its
/// colors, its dimensions, and any typography that strays from the defaults. A
/// slot usually points at a shared token, for example `ramp::backgroundColor`
/// resolves to `color::surfaceDeep`, yet it can be pinned to its own literal
/// whenever a detail wants fine control.
///
/// @par Naming
/// Shared tones read as what they are, such as `surface` or `border`. Component
/// members carry a kind suffix so colors and metrics stay legible side by side,
/// so `backgroundColor` and `borderColor` for colors against plain nouns like
/// `height` or `gap` for dimensions. A state, when one is needed, trails the
/// role as in `borderColorSelected`.

#include <QColor>
#include <QString>

namespace enzo::style {

/// Returns a copy of the color set to the given alpha from 0 to 255.
inline QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

/// Returns a copy of the color set to the given opacity from 0.0 to 1.0.
inline QColor withOpacity(QColor color, double opacity)
{
    color.setAlphaF(opacity);
    return color;
}

/// Returns the color as a CSS `rgba()` string for stylesheet injection.
///
/// Unlike QColor::name, this keeps the alpha channel, so translucent tokens
/// survive the trip into a stylesheet.
inline QString cssRgba(const QColor& color)
{
    return QString("rgba(%1,%2,%3,%4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alphaF());
}

/// Broad stroke tones reused across the interface.
namespace color {
/// Deepest background tone behind panels, nodes, and scroll troughs.
inline const QColor surfaceDeep = QColor("#1b1b1b");
/// Dimmer inset surface for alternate rows and menu bars.
inline const QColor surfaceDim = QColor("#242424");
/// Primary panel surface.
inline const QColor surface = QColor("#282828");
/// Subtle separators and faint outlines.
inline const QColor divider = QColor("#303030");
/// Default border and disabled fill.
inline const QColor border = QColor("#383838");
/// Muted body text and icons.
inline const QColor textMuted = QColor("#b3b3b3");
/// Brighter text for selected or active items.
inline const QColor textBright = QColor("#e6e6e6");
/// Pure white for primary marks on dark surfaces.
inline const QColor white = QColor("#ffffff");
/// Pure black for scrims and translucent overlays.
inline const QColor black = QColor("#000000");
/// Blue accent for active highlights.
inline const QColor accentBlue = QColor("#00bfff");
/// Soft blue for expression mode text.
inline const QColor accentBlueSoft = QColor("#8ab4f8");
/// Yellow used to mark the selected node.
inline const QColor selection = QColor("#fee046");
/// Red used for errors.
inline const QColor error = QColor("#ff4d50");
} // namespace color

/// Application wide typography defaults.
namespace font {
/// Default point size for interface text.
inline constexpr int size = 9;
} // namespace font

/// Shared parameter row metrics.
namespace parameter {
/// Height in pixels of a parameter row and its widgets.
inline constexpr int height = 23;
/// Corner rounding in pixels for parameter frames.
inline constexpr int borderRadius = 8;
/// Vertical gap in pixels between stacked parameters.
inline constexpr int gap = 3;
/// Opacity of a disabled parameter row.
inline constexpr double disabledOpacity = 0.4;
} // namespace parameter

/// Spacer parameter metrics.
namespace spacer {
/// Height in pixels of a spacer row.
inline constexpr int height = 12;
} // namespace spacer

/// Ramp curve editor look.
namespace ramp {
inline const QColor backgroundColor = color::surfaceDeep;
inline const QColor borderColor = color::border;
inline const QColor foregroundColor = color::textMuted; // curve stroke and handles
inline const QColor handleColorSelected = color::white;
inline const QColor squareColor = QColor("#808080");
inline const QColor curveFillTopColor = withAlpha(color::textMuted, 100);
inline const QColor curveFillBottomColor = withAlpha(color::textMuted, 10);
/// Height in pixels of the curve editor.
inline constexpr int height = 80;
} // namespace ramp

/// Slider track and value look.
namespace slider {
inline const QColor trackColor = color::border;
inline const QColor foregroundColor = color::textMuted;
inline const QColor expressionColor = color::accentBlueSoft;
inline const QColor expressionColorError = color::error;
} // namespace slider

/// Network node graphic look.
namespace node {
inline const QColor backgroundColor = color::surfaceDeep;
inline const QColor outlineColor = QColor("#353535");
inline const QColor outlineColorSelected = color::selection;
inline const QColor foregroundColor = color::white;
inline const QColor hoverColor = QColor("#666666");
} // namespace node

/// Node socket look.
namespace socket {
inline const QColor activeColor = color::white;
inline const QColor inactiveColor = QColor("#9f9f9f");
} // namespace socket

/// Edge graphic look.
namespace edge {
inline const QColor defaultColor = color::white;
inline const QColor flowColor = QColor("#ff4a4a");
} // namespace edge

/// Display flag button look.
namespace displayFlag {
inline const QColor disabledColor = color::border;
inline const QColor enabledColor = color::accentBlue;
inline const QColor hoveredColor = QColor("#666666");
inline const QColor hoveredEnabledColor = QColor("#1391ff");
} // namespace displayFlag

/// Dropdown widget look.
namespace dropdown {
inline const QColor backgroundColor = QColor("#202020");
inline const QColor borderColor = color::border;
inline const QColor foregroundColor = color::textMuted;
} // namespace dropdown

/// Popup list look.
namespace popup {
inline const QColor backgroundColor = color::surfaceDeep;
inline const QColor borderColor = color::border;
inline const QColor foregroundColor = color::textMuted;
inline const QColor foregroundColorSelected = color::textBright;
inline const QColor hoverColor = withAlpha(color::border, 153);
} // namespace popup

/// Menu look.
namespace menu {
inline const QColor chevronColor = color::textMuted;
} // namespace menu

/// Menu bar look.
namespace menuBar {
inline const QColor foregroundColor = color::textMuted;
inline const QColor highlightColor = color::surface;
} // namespace menuBar

/// Toggle switch look.
namespace boolSwitch {
inline const QColor offColor = color::border;
inline const QColor onColor = color::textMuted;
} // namespace boolSwitch

/// Main window and tooltip chrome.
namespace interface {
inline const QColor textColor = withOpacity(color::white, 0.8);
} // namespace interface

/// Parameters panel chrome.
namespace parametersPanel {
inline const QColor placeholderColor = withOpacity(color::white, 0.5); // "No Node Selected" label
} // namespace parametersPanel

/// Viewport overlay controls.
namespace viewportOverlay {
inline const QColor backgroundColor = withOpacity(color::black, 0.2);
} // namespace viewportOverlay

/// Geometry spreadsheet look.
namespace geometrySpreadsheet {
inline const QColor selectionColor = QColor("#414141"); // prim view row highlight
} // namespace geometrySpreadsheet

} // namespace enzo::style
