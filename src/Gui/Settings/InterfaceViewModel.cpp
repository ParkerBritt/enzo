#include "Gui/Settings/InterfaceViewModel.h"

#include <QSettings>

namespace enzo::ui {

namespace {

constexpr auto kScaleSetting = "interface/scale";
const QList<qreal> kScales = {0.75, 0.9, 1.0, 1.1, 1.25, 1.5, 2.0};
constexpr qreal kDefaultScale = 1.0;

constexpr auto kDurationScaleSetting = "interface/durationScale";
const QStringList kAnimationSpeedLabels = {"Off", "Fast", "Normal", "Slow"};
const QList<qreal> kDurationScales = {0.0, 0.5, 1.0, 1.5};
constexpr qreal kDefaultDurationScale = 1.0;

/// @brief Returns the index of a value in a list of presets, or of the default when the value is not listed.
int getPresetIndex(const QList<qreal>& presets, qreal value, qreal defaultValue)
{
    for (int index = 0; index < presets.size(); ++index)
        if (qAbs(presets[index] - value) < 0.001) return index;
    return presets.indexOf(defaultValue);
}

} // namespace

InterfaceViewModel::InterfaceViewModel(QObject* parent) : QObject(parent)
{
    const QSettings settings;

    const qreal storedScale = settings.value(kScaleSetting, kDefaultScale).toDouble();
    savedScaleIndex_ = getPresetIndex(kScales, storedScale, kDefaultScale);
    scaleIndex_ = savedScaleIndex_;

    const qreal storedDurationScale = settings.value(kDurationScaleSetting, kDefaultDurationScale).toDouble();
    savedAnimationSpeedIndex_ = getPresetIndex(kDurationScales, storedDurationScale, kDefaultDurationScale);
    animationSpeedIndex_ = savedAnimationSpeedIndex_;
}

QList<qreal> InterfaceViewModel::scales() const { return kScales; }

int InterfaceViewModel::scaleIndex() const { return scaleIndex_; }

void InterfaceViewModel::setScaleIndex(int index)
{
    if (index < 0 || index >= kScales.size() || index == scaleIndex_) return;
    scaleIndex_ = index;
    Q_EMIT scaleChanged();
    Q_EMIT hasPendingChangesChanged();
}

qreal InterfaceViewModel::scale() const { return kScales[scaleIndex_]; }

QStringList InterfaceViewModel::animationSpeedLabels() const { return kAnimationSpeedLabels; }

int InterfaceViewModel::animationSpeedIndex() const { return animationSpeedIndex_; }

void InterfaceViewModel::setAnimationSpeedIndex(int index)
{
    if (index < 0 || index >= kDurationScales.size() || index == animationSpeedIndex_) return;
    animationSpeedIndex_ = index;
    Q_EMIT animationSpeedChanged();
    Q_EMIT hasPendingChangesChanged();
}

qreal InterfaceViewModel::durationScale() const { return kDurationScales[animationSpeedIndex_]; }

bool InterfaceViewModel::hasPendingChanges() const
{
    const bool scaleChanged = scaleIndex_ != savedScaleIndex_;
    const bool animationSpeedChanged = animationSpeedIndex_ != savedAnimationSpeedIndex_;
    return scaleChanged || animationSpeedChanged;
}

void InterfaceViewModel::apply()
{
    if (!hasPendingChanges()) return;
    savedScaleIndex_ = scaleIndex_;
    savedAnimationSpeedIndex_ = animationSpeedIndex_;
    QSettings settings;
    settings.setValue(kScaleSetting, scale());
    settings.setValue(kDurationScaleSetting, durationScale());
    Q_EMIT hasPendingChangesChanged();
}

void InterfaceViewModel::revert()
{
    setScaleIndex(savedScaleIndex_);
    setAnimationSpeedIndex(savedAnimationSpeedIndex_);
}

} // namespace enzo::ui
