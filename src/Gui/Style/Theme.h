#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>

namespace enzo::ui
{

/// @brief Design tokens for the QML app, exposed to QML as the `Theme` object.
///
/// Lives as a context property so every QML context resolves `Theme.*`, including
/// view and header delegates and components reloaded during development.
class Theme : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QColor bg MEMBER bg_ CONSTANT)
    Q_PROPERTY(QColor panel MEMBER panel_ CONSTANT)
    Q_PROPERTY(QColor panel2 MEMBER panel2_ CONSTANT)
    Q_PROPERTY(QColor border MEMBER border_ CONSTANT)
    Q_PROPERTY(QColor bsoft MEMBER bsoft_ CONSTANT)
    Q_PROPERTY(QColor field MEMBER field_ CONSTANT)
    Q_PROPERTY(QColor fline MEMBER fline_ CONSTANT)
    Q_PROPERTY(QColor panelHeader MEMBER panelHeader_ CONSTANT)
    Q_PROPERTY(QColor text MEMBER text_ CONSTANT)
    Q_PROPERTY(QColor label MEMBER label_ CONSTANT)
    Q_PROPERTY(QColor muted MEMBER muted_ CONSTANT)
    Q_PROPERTY(QColor name MEMBER name_ CONSTANT)
    Q_PROPERTY(QColor accent MEMBER accent_ CONSTANT)
    Q_PROPERTY(QColor accent2 MEMBER accent2_ CONSTANT)
    Q_PROPERTY(QColor accentDim MEMBER accentDim_ CONSTANT)
    Q_PROPERTY(QColor accentLine MEMBER accentLine_ CONSTANT)
    Q_PROPERTY(QColor axisX MEMBER axisX_ CONSTANT)
    Q_PROPERTY(QColor axisY MEMBER axisY_ CONSTANT)
    Q_PROPERTY(QColor axisZ MEMBER axisZ_ CONSTANT)
    Q_PROPERTY(QColor axisXLight MEMBER axisXLight_ CONSTANT)
    Q_PROPERTY(QColor axisYLight MEMBER axisYLight_ CONSTANT)
    Q_PROPERTY(QColor axisZLight MEMBER axisZLight_ CONSTANT)
    Q_PROPERTY(QStringList modeColors MEMBER modeColors_ CONSTANT)
    Q_PROPERTY(QString fontUi MEMBER fontUi_ CONSTANT)
    Q_PROPERTY(QString fontMono MEMBER fontMono_ CONSTANT)
    Q_PROPERTY(QString iconsDir MEMBER iconsDir_ CONSTANT)

  public:
    explicit Theme(QObject* parent = nullptr);

  private:
    QColor bg_{"#0a0a0d"};
    QColor panel_{"#141418"};
    QColor panel2_{"#191920"};
    QColor border_{"#262630"};
    QColor bsoft_{"#1d1d25"};
    QColor field_{"#0d0d11"};
    QColor fline_{"#242430"};
    QColor panelHeader_{"#101015"};
    QColor text_{"#e7e8ec"};
    QColor label_{"#9a9aa6"};
    QColor muted_{"#63636d"};
    QColor name_{"#f6f6f9"};
    QColor accent_{"#8b5cf6"};
    QColor accent2_{"#a78bfa"};
    QColor accentDim_{QColor::fromRgbF(0.545f, 0.361f, 0.965f, 0.15f)};
    QColor accentLine_{QColor::fromRgbF(0.545f, 0.361f, 0.965f, 0.45f)};
    QColor axisX_{"#f5685f"};
    QColor axisY_{"#46c98a"};
    QColor axisZ_{"#4ea1ff"};
    QColor axisXLight_{"#f1857d"};
    QColor axisYLight_{"#7fd3a6"};
    QColor axisZLight_{"#7fb4f2"};
    QStringList modeColors_{"#4ea1ff", "#f0a93b", "#46c98a", "#f5685f", "#b69bf2"};
    QString fontUi_{"Public Sans"};
    QString fontMono_{"Inconsolata"};
    QString iconsDir_;
};

} // namespace enzo::ui
