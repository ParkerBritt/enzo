#include "Gui/Settings/InterfaceViewModel.h"

#include <QSettings>

namespace enzo::ui {

namespace {

constexpr auto kScaleSetting = "interface/scale";
const QList<qreal> kScales = {0.75, 0.9, 1.0, 1.1, 1.25, 1.5, 2.0};
constexpr qreal kDefaultScale = 1.0;

/// @brief Returns the index of a scale in kScales, or of the default scale when it is not listed.
int getScaleIndex(qreal scale)
{
    for (int index = 0; index < kScales.size(); ++index)
        if (qFuzzyCompare(kScales[index], scale)) return index;
    return kScales.indexOf(kDefaultScale);
}

} // namespace

InterfaceViewModel::InterfaceViewModel(QObject* parent) : QObject(parent)
{
    const qreal storedScale = QSettings().value(kScaleSetting, kDefaultScale).toDouble();
    savedScaleIndex_ = getScaleIndex(storedScale);
    scaleIndex_ = savedScaleIndex_;
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

bool InterfaceViewModel::hasPendingChanges() const { return scaleIndex_ != savedScaleIndex_; }

void InterfaceViewModel::apply()
{
    if (!hasPendingChanges()) return;
    savedScaleIndex_ = scaleIndex_;
    QSettings().setValue(kScaleSetting, scale());
    Q_EMIT hasPendingChangesChanged();
}

void InterfaceViewModel::revert() { setScaleIndex(savedScaleIndex_); }

} // namespace enzo::ui
